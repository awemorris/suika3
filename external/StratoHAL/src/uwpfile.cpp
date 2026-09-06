
/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * File HAL for UWP
 *
 * UWP version of stdfile.c:
 * - Uses Windows::Storage / RandomAccessStream instead of FILE*
 * - Supports packaged assets (HAL_PACKAGE_FILE)
 * - Supports obfuscated save files
 */

extern "C" {
#include <strato/strato.h>
}

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <algorithm>

#include <windows.h>
#include <wrl.h>
#include <ppltasks.h>

#include <windows.storage.h>
#include <windows.storage.streams.h>
#include <windows.applicationmodel.h>

using namespace concurrency;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

/*
 * The "key" of obfuscation
 */

 /* The key. */
#define OBFUSCATION_KEY	(0xabadcafedeadbeefULL)

/* The key is shuffled so that decompilers cannot read it directly. */
static volatile uint64_t key_obfuscated =
~((((OBFUSCATION_KEY >> 56) & 0xff) << 0) |
	(((OBFUSCATION_KEY >> 48) & 0xff) << 8) |
	(((OBFUSCATION_KEY >> 40) & 0xff) << 16) |
	(((OBFUSCATION_KEY >> 32) & 0xff) << 24) |
	(((OBFUSCATION_KEY >> 24) & 0xff) << 32) |
	(((OBFUSCATION_KEY >> 16) & 0xff) << 40) |
	(((OBFUSCATION_KEY >> 8) & 0xff) << 48) |
	(((OBFUSCATION_KEY >> 0) & 0xff) << 56));

/* Shuffled key. */
static volatile uint64_t key_reversed;

/* Use indirect addressing to avoid storing key_reserved to a register on x86. */
static volatile uint64_t* key_ref = &key_reversed;

/* These keys are not secret. */
static const uint64_t NEXT_MASK1 = 0xafcb8f2ff4fff33fULL;
static const uint64_t NEXT_MASK2 = 0xfcbfaff8f2f4f3f0ULL;

/*
 * Package
 */

#define ENTRY_SIZE		(65536)
#define FILE_NAME_SIZE		(256)

struct file_entry {
	char name[FILE_NAME_SIZE];
	uint64_t size;
	uint64_t offset;
};

static std::wstring g_installed_path;
static std::wstring g_local_path;
static std::wstring g_package_path;
static bool g_has_package;
static uint64_t entry_count;
static struct file_entry entry[ENTRY_SIZE];

/*
 * File read stream
 *
 * UWP version is fully memory-backed.
 */
struct hal_rfile {
	bool is_packaged;
	bool is_obfuscated;

	uint8_t* buf;
	size_t size;
	size_t pos;

	uint64_t index;
	uint64_t offset;

	uint64_t next_random;
	uint64_t prev_random;
};

/*
 * File write stream
 *
 * Buffered in memory and flushed on close.
 */
struct hal_wfile {
	std::wstring path;
	uint8_t* buf;
	size_t size;
	size_t capacity;
	uint64_t next_random;
};

/*
 * Forward declarations
 */
static bool open_package(struct hal_rfile* rf, const char* path);
static bool open_real(struct hal_rfile* rf, const char* path);
static void ungetc_rfile(struct hal_rfile* rf, char c);
static void set_random_seed(uint64_t index, uint64_t* next_random);
static char get_next_random(uint64_t* next_random, uint64_t* prev_random);
static void rewind_random(uint64_t* next_random, uint64_t* prev_random);

static bool is_save_file(const char* path);
static std::wstring utf8_to_utf16(const char* s);
static std::string utf16_to_utf8(const wchar_t* s);
static std::wstring make_asset_path(const char* path);
static std::wstring make_save_path(const char* path);

static bool ensure_save_parent_dirs(const std::wstring& path);
static bool ensure_save_directory_tree(const std::wstring& dir);
static bool file_exists(const std::wstring& path);
static bool read_all_bytes(const std::wstring& path, std::vector<uint8_t>& out);
static bool read_range_bytes(const std::wstring& path, uint64_t offset, uint64_t size, std::vector<uint8_t>& out);
static bool write_all_bytes(const std::wstring& path, const uint8_t* buf, size_t size);
static bool remove_file_by_path(const std::wstring& path);
static bool read_package_header(void);

/*
 * Initialize the file module.
 */
bool
init_file(void)
{
	try {
		g_installed_path = Package::Current->InstalledLocation->Path->Data();
		g_local_path = ApplicationData::Current->LocalFolder->Path->Data();
	}
	catch (...) {
		hal_log_error("Failed to acquire UWP file paths.");
		return false;
	}

	g_package_path = make_asset_path(HAL_PACKAGE_FILE);
	g_has_package = false;
	entry_count = 0;

	if (!file_exists(g_package_path)) {
		/* Package is optional on UWP too. */
		return true;
	}

	if (!read_package_header())
		return false;

	g_has_package = true;
	return true;
}

/*
 * Cleanup the file module.
 */
void
cleanup_file(void)
{
	g_has_package = false;
	entry_count = 0;
	g_package_path.clear();
}

/*
 * Check whether a file exists.
 */
bool
hal_check_file_exist(
	const char* file)
{
	uint64_t i;

	if (file == NULL)
		return false;

	if (g_has_package && !is_save_file(file)) {
		for (i = 0; i < entry_count; i++) {
			if (_stricmp(entry[i].name, file) == 0)
				return true;
		}
	}

	if (is_save_file(file))
		return file_exists(make_save_path(file));
	else
		return file_exists(make_asset_path(file));
}

/*
 * Open a read file stream.
 */
bool
hal_open_rfile(
	const char* path,
	struct hal_rfile** f)
{
	struct hal_rfile* rf;

	if (path == NULL || f == NULL)
		return false;

	rf = (struct hal_rfile*)malloc(sizeof(struct hal_rfile));
	if (rf == NULL) {
		hal_log_out_of_memory();
		return false;
	}
	memset(rf, 0, sizeof(*rf));

	if (g_has_package && !is_save_file(path)) {
		if (!open_package(rf, path)) {
			free(rf);
			return false;
		}
		*f = rf;
		return true;
	}

	if (!open_real(rf, path)) {
		free(rf);
		return false;
	}

	*f = rf;
	return true;
}

/* Open a file in the package. */
static bool
open_package(
	struct hal_rfile* rf,
	const char* path)
{
	uint64_t i;
	std::vector<uint8_t> data;

	for (i = 0; i < entry_count; i++) {
		if (_stricmp(entry[i].name, path) == 0)
			break;
	}
	if (i == entry_count)
		return false;

	if (!read_range_bytes(g_package_path, entry[i].offset, entry[i].size, data))
		return false;

	rf->buf = (uint8_t*)malloc(data.size() > 0 ? data.size() : 1);
	if (rf->buf == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	if (!data.empty())
		memcpy(rf->buf, data.data(), data.size());

	rf->is_packaged = true;
	rf->is_obfuscated = true;
	rf->size = data.size();
	rf->pos = 0;
	rf->index = i;
	rf->offset = entry[i].offset;
	set_random_seed(i, &rf->next_random);
	rf->prev_random = 0;

	return true;
}

/* Open a real file on a file system. */
static bool
open_real(
	struct hal_rfile* rf,
	const char* path)
{
	std::vector<uint8_t> data;
	std::wstring real_path;

	real_path = is_save_file(path) ? make_save_path(path) : make_asset_path(path);

	if (!read_all_bytes(real_path, data))
		return false;

	rf->buf = (uint8_t*)malloc(data.size() > 0 ? data.size() : 1);
	if (rf->buf == NULL) {
		hal_log_out_of_memory();
		return false;
	}

	if (!data.empty())
		memcpy(rf->buf, data.data(), data.size());

	rf->is_packaged = false;
	rf->is_obfuscated = false;
	rf->size = data.size();
	rf->pos = 0;
	rf->index = 0;
	rf->offset = 0;
	rf->next_random = 0;
	rf->prev_random = 0;

	return true;
}

/*
 * Get a file size.
 */
bool
hal_get_rfile_size(
	struct hal_rfile* f,
	size_t* ret)
{
	if (f == NULL || ret == NULL)
		return false;

	*ret = f->size;
	return true;
}

/*
 * Enable de-obfuscation on a read file stream.
 */
void
hal_decode_rfile(
	struct hal_rfile* f)
{
	if (f == NULL)
		return;

	f->is_obfuscated = true;
	set_random_seed(0, &f->next_random);
	f->prev_random = 0;
}

/*
 * Read bytes from a read file stream.
 */
bool
hal_read_rfile(
	struct hal_rfile* f,
	void* buf,
	size_t size,
	size_t* ret)
{
	size_t len, i;

	assert(f != NULL);
	assert(buf != NULL);
	assert(ret != NULL);

	if (f->pos >= f->size) {
		*ret = 0;
		return false;
	}

	len = size;
	if (f->pos + len > f->size)
		len = f->size - f->pos;

	if (len == 0) {
		*ret = 0;
		return false;
	}

	memcpy(buf, f->buf + f->pos, len);
	f->pos += len;

	if (f->is_obfuscated) {
		for (i = 0; i < len; i++)
			((uint8_t*)buf)[i] ^= (uint8_t)get_next_random(&f->next_random, &f->prev_random);
	}

	*ret = len;
	return true;
}

/*
 * Read a u64 from a file stream.
 */
bool
get_rfile_u64(
	struct hal_rfile* f,
	uint64_t* data)
{
	uint64_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 8, &ret))
		return false;
	if (ret != 8)
		return false;

	*data = hal_le_to_host_64(val);
	return true;
}

/*
 * Read a u32 from a file stream.
 */
bool
hal_get_rfile_u32(
	struct hal_rfile* f,
	uint32_t* data)
{
	uint32_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 4, &ret))
		return false;
	if (ret != 4)
		return false;

	*data = hal_le_to_host_32(val);
	return true;
}

/*
 * Read a u16 from a file stream.
 */
bool
hal_get_rfile_u16(
	struct hal_rfile* f,
	uint16_t* data)
{
	uint16_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 2, &ret))
		return false;
	if (ret != 2)
		return false;

	*data = hal_le_to_host_16(val);
	return true;
}

/*
 * Read a u8 from a file stream.
 */
bool
hal_get_rfile_u8(
	struct hal_rfile* f,
	uint8_t* data)
{
	uint8_t val;
	size_t ret;

	if (!hal_read_rfile(f, &val, 1, &ret))
		return false;
	if (ret != 1)
		return false;

	*data = val;
	return true;
}

/*
 * Read a line from a read file stream.
 */
bool
hal_get_rfile_string(
	struct hal_rfile* f,
	char* buf,
	size_t size)
{
	char* ptr;
	size_t len, read_size;
	char c;

	assert(f != NULL);
	assert(buf != NULL);
	assert(size > 0);

	ptr = buf;
	for (len = 0; len < size - 1; len++) {
		if (!hal_read_rfile(f, &c, 1, &read_size)) {
			*ptr = '\0';
			return len == 0 ? false : true;
		}
		if (c == '\n' || c == '\0') {
			*ptr = '\0';
			return true;
		}
		if (c == '\r') {
			if (!hal_read_rfile(f, &c, 1, &read_size)) {
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

	return len == 0 ? false : true;
}

/* Push back a character to a read file stream. */
static void
ungetc_rfile(
	struct hal_rfile* f,
	char c)
{
	UNUSED_PARAMETER(c);

	assert(f != NULL);
	assert(f->pos > 0);

	f->pos--;

	if (f->is_obfuscated)
		rewind_random(&f->next_random, &f->prev_random);
}

/*
 * Close a read file stream.
 */
void
hal_close_rfile(
	struct hal_rfile* f)
{
	if (f == NULL)
		return;

	if (f->buf != NULL)
		free(f->buf);
	free(f);
}

/*
 * Rewind a read file stream.
 */
void
hal_rewind_rfile(
	struct hal_rfile* f)
{
	assert(f != NULL);

	f->pos = 0;
	if (f->is_packaged)
		set_random_seed(f->index, &f->next_random);
	else if (f->is_obfuscated)
		set_random_seed(0, &f->next_random);
	else
		f->next_random = 0;
	f->prev_random = 0;
}

/* Set a random seed. */
static void
set_random_seed(
	uint64_t index,
	uint64_t* next_random)
{
	uint64_t i, next, lsb;

	key_reversed = ((((key_obfuscated >> 56) & 0xff) << 0) |
		(((key_obfuscated >> 48) & 0xff) << 8) |
		(((key_obfuscated >> 40) & 0xff) << 16) |
		(((key_obfuscated >> 32) & 0xff) << 24) |
		(((key_obfuscated >> 24) & 0xff) << 32) |
		(((key_obfuscated >> 16) & 0xff) << 40) |
		(((key_obfuscated >> 8) & 0xff) << 48) |
		(((key_obfuscated >> 0) & 0xff) << 56));
	next = ~(*key_ref);
	for (i = 0; i < index; i++) {
		next ^= NEXT_MASK1;
		lsb = next >> 63;
		next = (next << 1) | lsb;
	}

	*next_random = next;
}

/* Get a next random mask. */
static char
get_next_random(
	uint64_t* next_random,
	uint64_t* prev_random)
{
	uint64_t next;
	char ret;

	if (prev_random != NULL)
		*prev_random = *next_random;

	ret = (char)(*next_random);
	next = *next_random;
	next = (((~(*key_ref) & 0xff00) * next + (~(*key_ref) & 0xff)) %
		~(*key_ref)) ^ NEXT_MASK2;
	*next_random = next;

	return ret;
}

/* Go back to the previous random mask. */
static void
rewind_random(
	uint64_t* next_random,
	uint64_t* prev_random)
{
	*next_random = *prev_random;
	*prev_random = 0;
}

/*
 * Open a write file stream.
 */
bool
hal_open_wfile(
	const char* file,
	struct hal_wfile** wf)
{
	struct hal_wfile* w;
	std::wstring path;

	if (file == NULL || wf == NULL)
		return false;

	path = make_save_path(file);

	if (!ensure_save_parent_dirs(path))
		return false;

	w = (struct hal_wfile*)malloc(sizeof(struct hal_wfile));
	if (w == NULL) {
		hal_log_out_of_memory();
		return false;
	}
	memset(w, 0, sizeof(*w));

	w->path = path;
	w->buf = NULL;
	w->size = 0;
	w->capacity = 0;
	set_random_seed(0, &w->next_random);

	*wf = w;
	return true;
}

/*
 * Write bytes to a write file stream.
 */
bool
hal_write_wfile(
	struct hal_wfile* wf,
	const void* buf,
	size_t size,
	size_t* ret)
{
	const uint8_t* src;
	size_t i, new_size;
	uint8_t* new_buf;

	assert(wf != NULL);
	assert(buf != NULL);
	assert(ret != NULL);

	src = (const uint8_t*)buf;
	new_size = wf->size + size;

	if (new_size > wf->capacity) {
		size_t new_cap = wf->capacity == 0 ? 1024 : wf->capacity;
		while (new_cap < new_size)
			new_cap *= 2;

		new_buf = (uint8_t*)realloc(wf->buf, new_cap);
		if (new_buf == NULL) {
			hal_log_out_of_memory();
			*ret = 0;
			return false;
		}
		wf->buf = new_buf;
		wf->capacity = new_cap;
	}

	for (i = 0; i < size; i++)
		wf->buf[wf->size + i] = src[i] ^ (uint8_t)get_next_random(&wf->next_random, NULL);

	wf->size += size;
	*ret = size;
	return true;
}

/*
 * Close a write file stream.
 */
void
hal_close_wfile(
	struct hal_wfile* wf)
{
	if (wf == NULL)
		return;

	write_all_bytes(wf->path, wf->buf, wf->size);

	if (wf->buf != NULL)
		free(wf->buf);
	free(wf);
}

/*
 * Remove a real file.
 */
bool
hal_remove_file(
	const char* file)
{
	if (file == NULL)
		return false;

	return remove_file_by_path(make_save_path(file));
}

/*
 * Create a save directory.
 */
bool
make_save_directory(void)
{
	std::wstring path = g_local_path;
	path += L"\\";
	path += utf8_to_utf16(HAL_SAVE_DIR);
	return ensure_save_directory_tree(path);
}

/*
 * Get a real path of a file.
 */
char*
make_real_path(
	const char* fname)
{
	std::wstring wpath;
	std::string utf8;
	char* ret;

	if (fname == NULL)
		return NULL;

	wpath = is_save_file(fname) ? make_save_path(fname) : make_asset_path(fname);
	utf8 = utf16_to_utf8(wpath.c_str());

	ret = (char*)malloc(utf8.size() + 1);
	if (ret == NULL)
		return NULL;

	memcpy(ret, utf8.c_str(), utf8.size() + 1);
	return ret;
}

/* Check whether the path is under save directory. */
static bool
is_save_file(
	const char* path)
{
	const char* prefix = HAL_SAVE_DIR "/";

	if (path == NULL)
		return false;

	return strncmp(path, prefix, strlen(prefix)) == 0;
}

/* Convert UTF-8 to UTF-16. */
static std::wstring
utf8_to_utf16(
	const char* s)
{
	int len;
	std::wstring out;

	if (s == NULL)
		return L"";

	len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	if (len <= 0)
		return L"";

	out.resize((size_t)len - 1);
	if (len > 1)
		MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], len);

	return out;
}

/* Convert UTF-16 to UTF-8. */
static std::string
utf16_to_utf8(
	const wchar_t* s)
{
	int len;
	std::string out;

	if (s == NULL)
		return "";

	len = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
	if (len <= 0)
		return "";

	out.resize((size_t)len - 1);
	if (len > 1)
		WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], len, NULL, NULL);

	return out;
}

/* Make an absolute path for an asset file. */
static std::wstring
make_asset_path(
	const char* path)
{
	std::wstring ret = g_installed_path;
	ret += L"\\";
	ret += utf8_to_utf16(path);
	return ret;
}

/* Make an absolute path for a save file. */
static std::wstring
make_save_path(
	const char* path)
{
	std::wstring ret = g_local_path;
	ret += L"\\";
	ret += utf8_to_utf16(path);
	return ret;
}

/* Ensure parent directories exist for a save file. */
static bool
ensure_save_parent_dirs(
	const std::wstring& path)
{
	size_t pos;
	std::wstring dir;

	pos = path.find_last_of(L"\\/");
	if (pos == std::wstring::npos)
		return true;

	dir = path.substr(0, pos);
	return ensure_save_directory_tree(dir);
}

/* Ensure a save directory tree exists under LocalFolder. */
static bool
ensure_save_directory_tree(
	const std::wstring& dir)
{
	size_t prefix_len;
	std::wstring rel;
	std::wstring normalized;
	size_t start, pos;

	if (dir.empty())
		return false;

	normalized = dir;
	std::replace(normalized.begin(), normalized.end(), L'/', L'\\');

	if (normalized.size() < g_local_path.size())
		return false;
	if (_wcsnicmp(normalized.c_str(), g_local_path.c_str(), g_local_path.size()) != 0)
		return false;

	prefix_len = g_local_path.size();
	if (normalized.size() == prefix_len)
		return true;

	if (normalized[prefix_len] == L'\\')
		rel = normalized.substr(prefix_len + 1);
	else
		rel = normalized.substr(prefix_len);

	try {
		StorageFolder^ folder = ApplicationData::Current->LocalFolder;
		start = 0;

		while (start < rel.size()) {
			pos = rel.find(L'\\', start);
			std::wstring part = (pos == std::wstring::npos) ?
				rel.substr(start) :
				rel.substr(start, pos - start);
			if (!part.empty()) {
				folder = create_task(folder->CreateFolderAsync(
					ref new String(part.c_str()),
					CreationCollisionOption::OpenIfExists)).get();
			}
			if (pos == std::wstring::npos)
				break;
			start = pos + 1;
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

/* Check file existence. */
static bool
file_exists(
	const std::wstring& path)
{
	try {
		create_task(StorageFile::GetFileFromPathAsync(ref new String(path.c_str()))).get();
		return true;
	}
	catch (...) {
		return false;
	}
}

/* Read an entire file. */
static bool
read_all_bytes(
	const std::wstring& path,
	std::vector<uint8_t>& out)
{
	try {
		StorageFile^ file =
			create_task(StorageFile::GetFileFromPathAsync(ref new String(path.c_str()))).get();

		IBuffer^ buffer =
			create_task(FileIO::ReadBufferAsync(file)).get();

		out.resize(buffer->Length);
		if (buffer->Length > 0) {
			auto reader = DataReader::FromBuffer(buffer);
			Array<unsigned char>^ arr = ref new Array<unsigned char>(buffer->Length);
			reader->ReadBytes(arr);
			memcpy(out.data(), arr->Data, buffer->Length);
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

/* Read a byte range from a file. */
static bool
read_range_bytes(
	const std::wstring& path,
	uint64_t offset,
	uint64_t size,
	std::vector<uint8_t>& out)
{
	try {
		StorageFile^ file =
			create_task(StorageFile::GetFileFromPathAsync(ref new String(path.c_str()))).get();

		IRandomAccessStreamWithContentType^ stream =
			create_task(file->OpenReadAsync()).get();

		if (offset > stream->Size)
			return false;
		if (offset + size > stream->Size)
			return false;

		stream->Seek(offset);

		out.resize((size_t)size);
		if (size == 0)
			return true;

		IBuffer^ buffer =
			create_task(stream->ReadAsync(ref new Buffer((unsigned int)size),
				(unsigned int)size,
				InputStreamOptions::None)).get();

		if (buffer->Length != size)
			return false;

		auto reader = DataReader::FromBuffer(buffer);
		Array<unsigned char>^ arr = ref new Array<unsigned char>((unsigned int)size);
		reader->ReadBytes(arr);
		memcpy(out.data(), arr->Data, (size_t)size);

		return true;
	}
	catch (...) {
		return false;
	}
}

/* Write an entire file. */
static bool
write_all_bytes(
	const std::wstring& path,
	const uint8_t* buf,
	size_t size)
{
	try {
		StorageFolder^ local = ApplicationData::Current->LocalFolder;
		std::wstring normalized = path;
		std::replace(normalized.begin(), normalized.end(), L'/', L'\\');

		std::wstring rel = normalized;
		if (_wcsnicmp(normalized.c_str(), g_local_path.c_str(), g_local_path.size()) == 0) {
			if (normalized.size() > g_local_path.size() &&
				normalized[g_local_path.size()] == L'\\')
				rel = normalized.substr(g_local_path.size() + 1);
			else
				rel = normalized.substr(g_local_path.size());
		}

		size_t pos = rel.find_last_of(L'\\');
		StorageFolder^ folder = local;
		std::wstring name = rel;
		if (pos != std::wstring::npos) {
			std::wstring dir = rel.substr(0, pos);
			name = rel.substr(pos + 1);

			size_t start = 0;
			while (start < dir.size()) {
				size_t next = dir.find(L'\\', start);
				std::wstring part = (next == std::wstring::npos) ?
					dir.substr(start) :
					dir.substr(start, next - start);
				if (!part.empty()) {
					folder = create_task(folder->CreateFolderAsync(
						ref new String(part.c_str()),
						CreationCollisionOption::OpenIfExists)).get();
				}
				if (next == std::wstring::npos)
					break;
				start = next + 1;
			}
		}

		StorageFile^ file =
			create_task(folder->CreateFileAsync(
				ref new String(name.c_str()),
				CreationCollisionOption::ReplaceExisting)).get();

		Array<unsigned char>^ arr = ref new Array<unsigned char>((unsigned int)size);
		if (size > 0)
			memcpy(arr->Data, buf, size);

		create_task(FileIO::WriteBytesAsync(file, arr)).get();
		return true;
	}
	catch (...) {
		return false;
	}
}

/* Remove a file by path. */
static bool
remove_file_by_path(
	const std::wstring& path)
{
	try {
		StorageFile^ file =
			create_task(StorageFile::GetFileFromPathAsync(ref new String(path.c_str()))).get();
		create_task(file->DeleteAsync()).get();
		return true;
	}
	catch (...) {
		return false;
	}
}

/* Read and parse the package header. */
static bool
read_package_header(void)
{
	std::vector<uint8_t> head;
	const uint8_t* p;
	uint64_t i, next_random;
	size_t need;
	int j;

	if (!read_range_bytes(g_package_path, 0, sizeof(uint64_t), head))
		return false;
	if (head.size() != sizeof(uint64_t))
		return false;

	memcpy(&entry_count, head.data(), sizeof(uint64_t));
	entry_count = hal_le_to_host_64(entry_count);

	if (entry_count > ENTRY_SIZE) {
		hal_log_error("Corrupted package file.");
		return false;
	}

	need = (size_t)(entry_count * (FILE_NAME_SIZE + sizeof(uint64_t) + sizeof(uint64_t)));
	if (!read_range_bytes(g_package_path, sizeof(uint64_t), need, head))
		return false;
	if (head.size() != need)
		return false;

	p = head.data();
	for (i = 0; i < entry_count; i++) {
		memcpy(entry[i].name, p, FILE_NAME_SIZE);
		p += FILE_NAME_SIZE;

		set_random_seed(i, &next_random);
		for (j = 0; j < FILE_NAME_SIZE; j++)
			entry[i].name[j] ^= get_next_random(&next_random, NULL);

		memcpy(&entry[i].size, p, sizeof(uint64_t));
		p += sizeof(uint64_t);
		entry[i].size = hal_le_to_host_64(entry[i].size);

		memcpy(&entry[i].offset, p, sizeof(uint64_t));
		p += sizeof(uint64_t);
		entry[i].offset = hal_le_to_host_64(entry[i].offset);
	}

	return true;
}