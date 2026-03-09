/* -*- coding: utf-8; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
 * Playfield Engine
 * File HAL for Microsoft Game Development Kit (GDK)
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

extern "C" {
#include "stratohal/platform.h"
#include "gdkfile.h"
};

#include <windows.h>
#include <XPackage.h>
#include <XGameSaveFiles.h>
#include <XAsync.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

//
// File read stream.
//
struct hal_rfile {
    // Win32 file handle.
    HANDLE handle;

    // Single-byte pushback for get_string().
    bool has_ungetc;
    char ungetc_char;
};

//
// File write stream.
//
struct hal_wfile {
    // Win32 file handle.
    HANDLE handle;
};

//
// These must be provided by your user/login layer.
// - g_gdk_user: signed-in user used for saves
// - g_gdk_scid: service configuration id (SCID)
//
extern XUserHandle g_gdk_user;
extern const char *g_gdk_scid;

//
// Where save files live (obtained from XGameSaveFilesGetFolderWithUi*).
//
static char g_package_root[MAX_PATH];
static bool g_package_root_ready;
static XPackageMountHandle g_package_mount_handle;
static const char *package_path; // mount root if available (UTF-8/ANSI path)
static char g_save_root[MAX_PATH];
static bool g_save_root_ready;
static const char *k_save_container = "save"; // must be a sub-path

//
// Forward declarations.
//
static void ungetc_rfile(struct hal_rfile *rf, char c);
static bool ensure_save_root(void);
static bool build_save_path(char *dst, size_t dst_size, const char *relative);
static bool ensure_package_root(void);
static bool open_package(struct hal_rfile *rf, const char *relative);

bool
init_file(void)
{
    ensure_package_root();
    return true;
}

void
cleanup_file(void)
{
    // Best-effort close of the mount handle.
    if (g_package_root_ready && g_package_mount_handle != NULL) {
        XPackageCloseMountHandle(g_package_mount_handle);
        g_package_mount_handle = NULL;
    }
    g_package_root_ready = false;
}

//
// Check whether a file exists.
//
bool
hal_check_file_exist(
    const char *file)
{
    //
    // If package_path is set, we assume assets were mounted and package_path
    // points to the mount root. In that case, we can check existence using
    // regular Win32 attribute query.
    //
    if (package_path != NULL) {
        char full_asset[MAX_PATH];
        _snprintf_s(full_asset, sizeof(full_asset), _TRUNCATE, "%s\\%s", package_path, file);
        DWORD attrs = GetFileAttributesA(full_asset);
        return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
    }

    if (strncmp(file, "save/", 5) != 0) {
        DWORD attrs = GetFileAttributesA(file);
        return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
    }

    char full[MAX_PATH];
    if (!build_save_path(full, sizeof(full), file))
        return false;
    DWORD attrs = GetFileAttributesA(full);
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

//
// Open a read file stream.
//
bool
hal_open_rfile(
    const char *path,
    struct hal_rfile **f)
{
    struct hal_rfile *fs;

    fs = (struct hal_rfile *)malloc(sizeof(struct hal_rfile));
    if (fs == NULL) {
        log_out_of_memory();
        return false;
    }
    ZeroMemory(fs, sizeof(*fs));

    // If we're using a package file, open via package reader.
    if (package_path != NULL) {
        if (!open_package(fs, path)) {
            free(fs);
            return false;
        }
        *f = fs;
        return true;
    }

    // Non-save asset file.
    if (strncmp(path, "save/", 5) != 0) {
        fs->handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, NULL);
        if (fs->handle == INVALID_HANDLE_VALUE) {
            fs->handle = NULL;
            free(fs);
            return false;
        }
        *f = fs;
        return true;
    }

    // Save file: open under the XGameSaveFiles folder using Win32 I/O.
    char full[MAX_PATH];
    if (!build_save_path(full, sizeof(full), path)) {
        free(fs);
        return false;
    }

    fs->handle = CreateFileA(full, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (fs->handle == INVALID_HANDLE_VALUE) {
        fs->handle = NULL;
        free(fs);
        return false;
    }
    *f = fs;
    return true;
}

// Open a packaged asset under package_path.
static bool
open_package(
    struct hal_rfile *rf,
    const char *relative)
{
    assert(rf != NULL);
    assert(relative != NULL);

    if (!ensure_package_root() || package_path == NULL)
        return false;

    char full_asset[MAX_PATH];
    _snprintf_s(full_asset, sizeof(full_asset), _TRUNCATE, "%s\\%s", package_path, relative);

    rf->handle = CreateFileA(full_asset, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (rf->handle == INVALID_HANDLE_VALUE) {
        rf->handle = NULL;
        return false;
    }

    return true;
}

static bool
ensure_package_root(void)
{
    if (g_package_root_ready)
        return true;

    ZeroMemory(g_package_root, sizeof(g_package_root));
    g_package_mount_handle = NULL;

    // 1) Try XPackage APIs when we're packaged.
    if (XPackageIsPackagedProcess()) {
        char id[XPACKAGE_IDENTIFIER_MAX_LENGTH];
        ZeroMemory(id, sizeof(id));
        HRESULT hr = XPackageGetCurrentProcessPackageIdentifier(ARRAYSIZE(id), id);
        if (SUCCEEDED(hr) && id[0] != '\0') {
            hr = XPackageMount(id, &g_package_mount_handle);
            if (SUCCEEDED(hr) && g_package_mount_handle != NULL) {
                size_t pathSize = 0;
                hr = XPackageGetMountPathSize(g_package_mount_handle, &pathSize);
                if (SUCCEEDED(hr) && pathSize > 0 && pathSize <= ARRAYSIZE(g_package_root)) {
                    hr = XPackageGetMountPath(g_package_mount_handle, ARRAYSIZE(g_package_root), g_package_root);
                    if (SUCCEEDED(hr) && g_package_root[0] != '\0') {
                        package_path = g_package_root;
                        g_package_root_ready = true;
                        return true;
                    }
                }

                /* If we couldn't get the path, close the mount handle. */
                if (g_package_mount_handle != NULL) {
                    XPackageCloseMountHandle(g_package_mount_handle);
                    g_package_mount_handle = NULL;
                }
            }
        }
    }

    // 2) Fallback: use the executable's directory.
    char exePath[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, exePath, ARRAYSIZE(exePath));
    if (n == 0 || n >= ARRAYSIZE(exePath)) {
        log_error("GetModuleFileNameA failed.");
        return false;
    }
    // Strip filename.
    for (int i = (int)n - 1; i >= 0; i--) {
        if (exePath[i] == '\\' || exePath[i] == '/') {
            exePath[i] = '\0';
            break;
        }
    }
    strncpy_s(g_package_root, sizeof(g_package_root), exePath, _TRUNCATE);
    package_path = g_package_root;
    g_package_root_ready = true;
    return true;
}

static bool
build_save_path(
    char *dst,
    size_t dst_size,
    const char *relative)
{
    if (!ensure_save_root()) {
        _snprintf_s(dst, dst_size, _TRUNCATE, "save\\%s", relative);
        return true;
    }

    // Store everything under the container subdir to be cloud-synced.
    _snprintf_s(dst, dst_size, _TRUNCATE, "%s\\%s\\%s", g_save_root, k_save_container, relative);
    return true;
}

static bool
ensure_save_root(void)
{
    if (g_save_root_ready)
        return true;

    if (g_gdk_user == NULL || g_gdk_scid == NULL || g_gdk_scid[0] == '\0') {
        log_error("GDK save root not available: user or SCID not set.");
        return false;
    }

    //
    // XGameSaveFilesGetFolderWithUiAsync returns a folder path (UTF-8 char*)
    // where we can create subfolders/files using Win32 I/O.
    //
    HANDLE evt = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (evt == NULL) {
        log_error("CreateEvent failed.");
        return false;
    }

    XAsyncBlock async;
    ZeroMemory(&async, sizeof(async));
    async.context = evt;
    async.callback = [](XAsyncBlock *ab) {
        SetEvent((HANDLE)ab->context);
    };

    HRESULT hr = XGameSaveFilesGetFolderWithUiAsync(g_gdk_user, g_gdk_scid, &async);
    if (FAILED(hr)) {
        CloseHandle(evt);
        log_error("XGameSaveFilesGetFolderWithUiAsync failed. hr=0x%08x", (unsigned)hr);
        return false;
    }

    WaitForSingleObject(evt, INFINITE);
    CloseHandle(evt);

    hr = XAsyncGetStatus(&async, false);
    if (FAILED(hr)) {
        log_error("XGameSaveFilesGetFolderWithUiAsync status failed. hr=0x%08x", (unsigned)hr);
        return false;
    }

    ZeroMemory(g_save_root, sizeof(g_save_root));
    hr = XGameSaveFilesGetFolderWithUiResult(&async, ARRAYSIZE(g_save_root), g_save_root);
    if (FAILED(hr) || g_save_root[0] == '\0') {
        log_error("XGameSaveFilesGetFolderWithUiResult failed. hr=0x%08x", (unsigned)hr);
        return false;
    }

    // Ensure container subdirectory exists: <root>/<container>/
    char container_path[MAX_PATH];
    _snprintf_s(container_path, sizeof(container_path), _TRUNCATE, "%s\\%s", g_save_root, k_save_container);
    CreateDirectoryA(container_path, NULL);

    g_save_root_ready = true;
    return true;
}

//
// Enable de-obfuscation on a read file stream.
//
void
hal_decode_rfile(
    struct hal_rfile *f)
{
    UNUSED_PARAMETER(f);
}

//
// Get a file size.
//
bool
hal_get_rfile_size(
    struct hal_rfile *f,
    size_t *ret)
{
    assert(f != NULL);
    assert(f->handle != NULL);

    LARGE_INTEGER sz;
    if (!GetFileSizeEx(f->handle, &sz))
        return false;

    if (sz.QuadPart < 0)
        return false;

    *ret = (size_t)sz.QuadPart;
    return true;
}

//
// Read bytes from a read file stream.
//
bool
hal_read_rfile(
    struct hal_rfile *f,
    void *buf,
    size_t size,
    size_t *ret)
{
    assert(f != NULL);
    assert(f->handle != NULL);

    char *dst = (char *)buf;
    size_t total = 0;

    // Serve a pending ungetc byte first.
    if (f->has_ungetc && size > 0) {
        dst[0] = f->ungetc_char;
        f->has_ungetc = false;
        total = 1;
        dst++;
        size--;
    }

    if (size > 0) {
        DWORD readBytes = 0;
        if (!ReadFile(f->handle, dst, (DWORD)size, &readBytes, NULL)) {
            *ret = total;
            return false;
        }
        total += (size_t)readBytes;
    }

    *ret = total;
    return total != 0;
}

//
// Read a u64 from a file stream.
//
bool
hal_get_rfile_u64(
    struct hal_rfile *f,
    uint64_t *data)
{
    uint64_t val;
    size_t ret;

    if (!read_rfile(f, &val, 8, &ret))
        return false;
    if (ret != 8)
        return false;

    *data = LETOHOST64(val);
    return true;
}

// Read a u32 from a file stream.
bool
hal_get_rfile_u32(
    struct hal_rfile *f,
    uint32_t *data)
{
    uint32_t val;
    size_t ret;

    if (!read_rfile(f, &val, 4, &ret))
        return false;
    if (ret != 4)
        return false;

    *data = LETOHOST32(val);
    return true;
}

// Read a u16 from a file stream.
bool
hal_get_rfile_u16(
    struct hal_rfile *f,
    uint16_t *data)
{
    uint16_t val;
    size_t ret;

    if (!read_rfile(f, &val, 2, &ret))
        return false;
    if (ret != 2)
        return false;

    *data = LETOHOST16(val);
    return true;
}

// Read a u8 from a file stream.
bool
hal_get_rfile_u8(
    struct hal_rfile *f,
    uint8_t *data)
{
    uint8_t val;
    size_t ret;

    if (!read_rfile(f, &val, 1, &ret))
        return false;
    if (ret != 1)
        return false;

    *data = val;
    return true;
}

//
// Read a line from a read file stream.
//
bool
hal_get_rfile_string(
    struct hal_rfile *f,
    char *buf,
    size_t size)
{
    char *ptr;
    size_t len, read_size;
    char c;

    assert(f != NULL);
    assert(f->handle != NULL);
    assert(buf != NULL);
    assert(size > 0);

    ptr = buf;
    for (len = 0; len < size - 1; len++) {
        if (!read_rfile(f, &c, 1, &read_size)) {
            *ptr = '\0';
            if (len == 0)
                return false;
            return true;
        }
        if (c == '\n' || c == '\0') {
            *ptr = '\0';
            return true;
        }
        if (c == '\r') {
            if (!read_rfile(f, &c, 1, &read_size)) {
                *ptr = '\0';
                return true;
            }
            if (c == '\n') {
                *ptr = '\0';
                return true;
            }
            ungetc_rfile(f, c);
            *ptr = '\0';
            return true;
        }
        *ptr++ = c;
    }
    *ptr = '\0';

    if (len == 0)
        return false;

    return true;
}

// Push back a character to a read file stream.
static void
ungetc_rfile(
    struct hal_rfile *f,
    char c)
{
    assert(f != NULL);
    assert(f->handle != NULL);

    // Single-byte pushback only.
    f->has_ungetc = true;
    f->ungetc_char = c;
}

//
// Close a read file stream.
//
void
hal_close_rfile(
    struct hal_rfile *f)
{
    assert(f != NULL);
    assert(f->handle != NULL);

    if (f->handle != NULL) {
        CloseHandle(f->handle);
        f->handle = NULL;
    }

    free(f);
}

//
// Rewind a read file stream.
//
void
hal_rewind_rfile(
    struct hal_rfile *f)
{
    assert(f != NULL);
    assert(f->handle != NULL);

    SetFilePointer(f->handle, 0, NULL, FILE_BEGIN);
    f->has_ungetc = false;
}

//
// Write
//

//
// Open a write file stream.
//
bool
hal_open_wfile(
    const char *file,
    struct hal_wfile **wf)
{
    char full[MAX_PATH];

    *wf = (struct wfile *)malloc(sizeof(struct wfile));
    if (*wf == NULL) {
        log_out_of_memory();
        return false;
    }
    ZeroMemory(*wf, sizeof(**wf));

    if (!build_save_path(full, sizeof(full), file)) {
        free(*wf);
        *wf = NULL;
        return false;
    }

    (*wf)->handle = CreateFileA(full, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    if ((*wf)->handle == INVALID_HANDLE_VALUE) {
        (*wf)->handle = NULL;
        free(*wf);
        *wf = NULL;
        return false;
    }

    return true;
}

//
// Write bytes to a write file stream.
//
bool
hal_write_wfile(
    struct wfile *wf,
    const void *buf,
    size_t size,
    size_t *ret)
{
    assert(wf != NULL);
    assert(wf->handle != NULL);

    const uint8_t *src = (const uint8_t *)buf;
    size_t total = 0;

    while (total < size) {
        DWORD chunk = (DWORD)((size - total) > 0x7fffffff ? 0x7fffffff : (size - total));
        DWORD written = 0;
        if (!WriteFile(wf->handle, src + total, chunk, &written, NULL)) {
            *ret = total;
            return false;
        }
        total += (size_t)written;
        if (written == 0)
            break;
    }

    *ret = total;
    return total == size;
}

//
// Close a write file stream.
//
void
hal_close_wfile(
    struct hal_wfile *wf)
{
    assert(wf != NULL);

    if (wf->handle != NULL) {
        FlushFileBuffers(wf->handle);
        CloseHandle(wf->handle);
        wf->handle = NULL;
    }
    free(wf);
}

//
// Remove a real file.
//
void
hal_remove_file(
    const char *file)
{
    UNUSED_PARAMETER(file);

    // Not supported (cloud saves are managed).
}
