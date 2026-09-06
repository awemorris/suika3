/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Immutable NWT1 model-weight reader. */

#include <noct/noct.h>
#include "runtime.h"
#include "objectmodel.h"
#include "sha256.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WEIGHTS_HANDLE_MAGIC 0x4e575431U
#define NWT1_HEADER_SIZE 104U
#define NWT1_DTYPE_FLOAT32 1U

struct nwt1_entry {
	char *name;
	uint8_t rank;
	uint64_t dimension[8];
	uint64_t payload_offset;
	uint64_t byte_length;
};

struct weights_handle {
	uint32_t magic;
	struct rt_vm *owner;
	FILE *file;
	bool closed;
	uint64_t payload_start;
	uint64_t payload_bytes;
	uint32_t entry_count;
	uint32_t initialized_count;
	struct nwt1_entry *entry;
};

static bool cfunc_Weights_open(NoctEnv *env);
static bool cfunc_Weights_loadFloat32(NoctEnv *env);
static bool cfunc_Weights_close(NoctEnv *env);
static void weights_finalizer(void *native_pointer);

static uint16_t
read_u16le(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t
read_u32le(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
	       ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t
read_u64le(const uint8_t *p)
{
	uint64_t value;
	int i;
	value = 0;
	for (i = 0; i < 8; i++) value |= (uint64_t)p[i] << (i * 8);
	return value;
}

static bool
add_u64(uint64_t a, uint64_t b, uint64_t *result)
{
	if (a > UINT64_MAX - b) return false;
	*result = a + b;
	return true;
}

static bool
align_u64(uint64_t value, uint64_t alignment, uint64_t *result)
{
	uint64_t remainder, addition;
	remainder = value % alignment;
	addition = remainder == 0 ? 0 : alignment - remainder;
	return add_u64(value, addition, result);
}

static bool
seek_file(FILE *file, uint64_t offset)
{
	if (offset > (uint64_t)LONG_MAX) return false;
	return fseek(file, (long)offset, SEEK_SET) == 0;
}

static bool
read_exact(FILE *file, void *buffer, size_t size)
{
	return size == 0 || fread(buffer, 1, size, file) == size;
}

static bool
valid_lower_hex_sha256(const char *text)
{
	size_t i;
	if (strlen(text) != 64) return false;
	for (i = 0; i < 64; i++)
		if (!((text[i] >= '0' && text[i] <= '9') ||
		      (text[i] >= 'a' && text[i] <= 'f'))) return false;
	return true;
}

static bool
valid_utf8_name(const uint8_t *s, size_t size)
{
	size_t i, needed;
	uint32_t cp, min;
	uint8_t c;
	i = 0;
	while (i < size) {
		c = s[i++];
		if (c == 0) return false;
		if (c < 0x80) continue;
		if ((c & 0xe0U) == 0xc0U) {
			needed = 1; cp = c & 0x1fU; min = 0x80U;
		} else if ((c & 0xf0U) == 0xe0U) {
			needed = 2; cp = c & 0x0fU; min = 0x800U;
		} else if ((c & 0xf8U) == 0xf0U) {
			needed = 3; cp = c & 0x07U; min = 0x10000U;
		} else return false;
		if (needed > size - i) return false;
		while (needed-- != 0) {
			c = s[i++];
			if ((c & 0xc0U) != 0x80U) return false;
			cp = (cp << 6) | (c & 0x3fU);
		}
		if (cp < min || cp > 0x10ffffU ||
		    (cp >= 0xd800U && cp <= 0xdfffU)) return false;
	}
	return true;
}

static bool
hash_file(FILE *file, uint64_t offset, uint64_t size, uint8_t digest[32])
{
	struct noct_sha256 ctx;
	uint8_t buffer[8192];
	uint64_t remaining;
	size_t chunk;
	if (!seek_file(file, offset)) return false;
	noct_sha256_init(&ctx);
	remaining = size;
	while (remaining != 0) {
		chunk = remaining > sizeof(buffer) ? sizeof(buffer) : (size_t)remaining;
		if (!read_exact(file, buffer, chunk)) return false;
		noct_sha256_update(&ctx, buffer, chunk);
		remaining -= chunk;
	}
	noct_sha256_final(&ctx, digest);
	return true;
}

static bool
zero_range(FILE *file, uint64_t offset, uint64_t size)
{
	uint8_t buffer[4096];
	uint64_t remaining;
	size_t chunk, i;
	if (!seek_file(file, offset)) return false;
	remaining = size;
	while (remaining != 0) {
		chunk = remaining > sizeof(buffer) ? sizeof(buffer) : (size_t)remaining;
		if (!read_exact(file, buffer, chunk)) return false;
		for (i = 0; i < chunk; i++) if (buffer[i] != 0) return false;
		remaining -= chunk;
	}
	return true;
}

static void
weights_close_contents(struct weights_handle *handle)
{
	uint32_t i;
	if (handle->closed) return;
	if (handle->file != NULL) (void)fclose(handle->file);
	handle->file = NULL;
	if (handle->entry != NULL)
		for (i = 0; i < handle->initialized_count; i++)
			noct_free(handle->entry[i].name);
	noct_free(handle->entry);
	handle->entry = NULL;
	handle->entry_count = 0;
	handle->initialized_count = 0;
	handle->closed = true;
}

static void
weights_finalizer(void *native_pointer)
{
	struct weights_handle *handle;
	handle = (struct weights_handle *)native_pointer;
	if (handle == NULL) return;
	if (handle->magic == WEIGHTS_HANDLE_MAGIC) {
		weights_close_contents(handle);
		handle->magic = 0;
	}
	noct_free(handle);
}

static bool
get_weights_handle(NoctEnv *env, NoctValue *value,
		   struct weights_handle **result, bool allow_closed)
{
	void *native_pointer;
	void (*finalizer)(void *);
	struct weights_handle *handle;
	if (!noct_get_dict_native_pointer(env, value, &native_pointer, &finalizer))
		return false;
	if (finalizer != weights_finalizer || native_pointer == NULL) {
		noct_error(env, "Weights handle kind mismatch.");
		return false;
	}
	handle = (struct weights_handle *)native_pointer;
	if (handle->magic != WEIGHTS_HANDLE_MAGIC) {
		noct_error(env, "Weights handle is invalid.");
		return false;
	}
	if (handle->owner != env->vm) {
		noct_error(env, "Weights handle belongs to a different VM.");
		return false;
	}
	if (!allow_closed && handle->closed) {
		noct_error(env, "Weights handle is closed.");
		return false;
	}
	*result = handle;
	return true;
}

static int
compare_entry_offset(const void *left, const void *right)
{
	const struct nwt1_entry *const *a;
	const struct nwt1_entry *const *b;
	a = (const struct nwt1_entry *const *)left;
	b = (const struct nwt1_entry *const *)right;
	if ((*a)->payload_offset < (*b)->payload_offset) return -1;
	if ((*a)->payload_offset > (*b)->payload_offset) return 1;
	return 0;
}

static bool
parse_directory(NoctEnv *env, struct weights_handle *handle,
		uint64_t directory_bytes)
{
	uint64_t consumed, remaining, expected_size, product, end;
	uint32_t i, j, entry_bytes, flags;
	uint16_t name_bytes;
	uint8_t *buffer;
	size_t base_size, k;
	struct nwt1_entry *entry;

	consumed = 0;
	for (i = 0; i < handle->entry_count; i++) {
		uint8_t size_bytes[4];
		remaining = directory_bytes - consumed;
		if (remaining < 4 || !read_exact(handle->file, size_bytes, 4)) {
			noct_error(env, "NWT1 directory is truncated.");
			return false;
		}
		entry_bytes = read_u32le(size_bytes);
		if (entry_bytes < 40 || (entry_bytes & 7U) != 0 ||
		    (uint64_t)entry_bytes > remaining) {
			noct_error(env, "NWT1 entry size is invalid.");
			return false;
		}
		buffer = noct_malloc(entry_bytes);
		if (buffer == NULL) { noct_error(env, "Out of memory."); return false; }
		memcpy(buffer, size_bytes, 4);
		if (!read_exact(handle->file, buffer + 4, entry_bytes - 4)) {
			noct_free(buffer);
			noct_error(env, "NWT1 directory is truncated.");
			return false;
		}
		name_bytes = read_u16le(buffer + 4);
		flags = read_u32le(buffer + 8);
		entry = &handle->entry[i];
		entry->rank = buffer[7];
		entry->payload_offset = read_u64le(buffer + 12);
		entry->byte_length = read_u64le(buffer + 20);
		if (name_bytes == 0 || buffer[6] != NWT1_DTYPE_FLOAT32 ||
		    entry->rank == 0 || entry->rank > 8 || flags != 0) {
			noct_free(buffer);
			noct_error(env, "NWT1 entry metadata is invalid.");
			return false;
		}
		base_size = 28 + (size_t)entry->rank * 8 + name_bytes;
		expected_size = (uint64_t)((base_size + 7) & ~(size_t)7);
		if (expected_size != entry_bytes) {
			noct_free(buffer);
			noct_error(env, "NWT1 entry length is not canonical.");
			return false;
		}
		product = 1;
		for (j = 0; j < entry->rank; j++) {
			entry->dimension[j] = read_u64le(buffer + 28 + (size_t)j * 8);
			if (entry->dimension[j] == 0 ||
			    product > (uint64_t)INT_MAX / entry->dimension[j]) {
				noct_free(buffer);
				noct_error(env, "NWT1 tensor shape is out-of-range.");
				return false;
			}
			product *= entry->dimension[j];
		}
		if (product > UINT64_MAX / 4 || entry->byte_length != product * 4 ||
		    (entry->payload_offset & 63U) != 0 ||
		    !add_u64(entry->payload_offset, entry->byte_length, &end) ||
		    end > handle->payload_bytes) {
			noct_free(buffer);
			noct_error(env, "NWT1 tensor payload range is invalid.");
			return false;
		}
		if (!valid_utf8_name(buffer + 28 + (size_t)entry->rank * 8,
				     name_bytes)) {
			noct_free(buffer);
			noct_error(env, "NWT1 tensor name is not valid UTF-8.");
			return false;
		}
		entry->name = noct_malloc((size_t)name_bytes + 1);
		if (entry->name == NULL) {
			noct_free(buffer); noct_error(env, "Out of memory."); return false;
		}
		memcpy(entry->name, buffer + 28 + (size_t)entry->rank * 8,
		       name_bytes);
		entry->name[name_bytes] = '\0';
		handle->initialized_count = i + 1;
		if (i != 0 && strcmp(handle->entry[i - 1].name, entry->name) >= 0) {
			noct_free(buffer);
			noct_error(env, "NWT1 tensor names are duplicate or out-of-order.");
			return false;
		}
		for (k = base_size; k < entry_bytes; k++) {
			if (buffer[k] != 0) {
				noct_free(buffer);
				noct_error(env, "NWT1 entry padding is not zero.");
				return false;
			}
		}
		noct_free(buffer);
		consumed += entry_bytes;
	}
	if (consumed != directory_bytes) {
		noct_error(env, "NWT1 directory byte count is inconsistent.");
		return false;
	}
	return true;
}

static bool
validate_payload_layout(NoctEnv *env, struct weights_handle *handle)
{
	struct nwt1_entry **sorted;
	uint64_t cursor, end;
	uint32_t i;
	if (handle->entry_count == 0)
		return zero_range(handle->file, handle->payload_start,
				  handle->payload_bytes);
	if (handle->entry_count != 0 &&
	    sizeof(*sorted) > SIZE_MAX / (size_t)handle->entry_count) {
		noct_error(env, "NWT1 entry table is too large."); return false;
	}
	sorted = noct_malloc((size_t)handle->entry_count * sizeof(*sorted));
	if (sorted == NULL) { noct_error(env, "Out of memory."); return false; }
	for (i = 0; i < handle->entry_count; i++) sorted[i] = &handle->entry[i];
	qsort(sorted, handle->entry_count, sizeof(*sorted), compare_entry_offset);
	cursor = 0;
	for (i = 0; i < handle->entry_count; i++) {
		if (sorted[i]->payload_offset < cursor) {
			noct_free(sorted);
			noct_error(env, "NWT1 tensor payloads overlap.");
			return false;
		}
		if (!zero_range(handle->file, handle->payload_start + cursor,
				sorted[i]->payload_offset - cursor)) {
			noct_free(sorted);
			noct_error(env, "NWT1 payload padding is not zero.");
			return false;
		}
		end = sorted[i]->payload_offset + sorted[i]->byte_length;
		cursor = end;
	}
	if (!zero_range(handle->file, handle->payload_start + cursor,
			  handle->payload_bytes - cursor)) {
		noct_free(sorted);
		noct_error(env, "NWT1 payload padding is not zero.");
		return false;
	}
	noct_free(sorted);
	return true;
}

static bool
parse_nwt1(NoctEnv *env, struct weights_handle *handle, uint64_t file_size)
{
	uint8_t header[NWT1_HEADER_SIZE];
	uint8_t digest[32];
	uint16_t major, minor;
	uint32_t header_bytes, flags;
	uint64_t directory_bytes, sections_end, directory_end;

	if (!seek_file(handle->file, 0) || !read_exact(handle->file, header, sizeof(header))) {
		noct_error(env, "NWT1 header is truncated."); return false;
	}
	if (memcmp(header, "NOCTWGT\0", 8) != 0) {
		noct_error(env, "NWT1 magic is invalid."); return false;
	}
	major = read_u16le(header + 8);
	minor = read_u16le(header + 10);
	header_bytes = read_u32le(header + 12);
	flags = read_u32le(header + 16);
	handle->entry_count = read_u32le(header + 20);
	directory_bytes = read_u64le(header + 24);
	handle->payload_bytes = read_u64le(header + 32);
	if (major != 1 || minor != 0 || header_bytes != NWT1_HEADER_SIZE || flags != 0) {
		noct_error(env, "NWT1 header version or reserved fields are invalid.");
		return false;
	}
	if (!add_u64(header_bytes, directory_bytes, &directory_end) ||
	    !align_u64(directory_end, 64, &handle->payload_start) ||
	    !add_u64(handle->payload_start, handle->payload_bytes, &sections_end) ||
	    sections_end != file_size || handle->payload_start > (uint64_t)LONG_MAX) {
		noct_error(env, "NWT1 section sizes are invalid.");
		return false;
	}
	if ((uint64_t)handle->entry_count > directory_bytes / 40U) {
		noct_error(env, "NWT1 entry count exceeds the directory size.");
		return false;
	}
	if (handle->entry_count != 0 && sizeof(*handle->entry) >
	    SIZE_MAX / (size_t)handle->entry_count) {
		noct_error(env, "NWT1 entry count is too large."); return false;
	}
	if (handle->entry_count != 0) {
		handle->entry = noct_calloc(handle->entry_count, sizeof(*handle->entry));
		if (handle->entry == NULL) { noct_error(env, "Out of memory."); return false; }
	}
	if (!seek_file(handle->file, header_bytes) ||
	    !parse_directory(env, handle, directory_bytes)) return false;
	if (!zero_range(handle->file, directory_end,
			  handle->payload_start - directory_end)) {
		noct_error(env, "NWT1 section padding is not zero."); return false;
	}
	if (!hash_file(handle->file, handle->payload_start,
		       handle->payload_bytes, digest) ||
	    memcmp(digest, header + 72, 32) != 0) {
		noct_error(env, "NWT1 payload SHA-256 mismatch."); return false;
	}
	if (!validate_payload_layout(env, handle)) return false;
	return true;
}

static bool
cfunc_Weights_open(NoctEnv *env)
{
	NoctValue path_value, expected_value, ret;
	const char *path, *expected;
	struct weights_handle *handle;
	uint8_t digest[32];
	char actual[65];
	long file_size_long;
	bool installed, ok;

	handle = NULL; installed = false; ok = false;
	if (!noct_pin_local(env, 3, &path_value, &expected_value, &ret)) return false;
	if (!noct_get_arg_check_string(env, 0, &path_value, &path) ||
	    !noct_get_arg_check_string(env, 1, &expected_value, &expected)) goto cleanup;
	if (!valid_lower_hex_sha256(expected)) {
		noct_error(env, "Expected pack SHA-256 must be 64 lowercase hex digits.");
		goto cleanup;
	}
	handle = noct_calloc(1, sizeof(*handle));
	if (handle == NULL) { noct_error(env, "Out of memory."); goto cleanup; }
	handle->magic = WEIGHTS_HANDLE_MAGIC;
	handle->owner = env->vm;
	handle->file = fopen(path, "rb");
	if (handle->file == NULL) { noct_error(env, "Cannot open file %s.", path); goto cleanup; }
	if (fseek(handle->file, 0, SEEK_END) != 0 ||
	    (file_size_long = ftell(handle->file)) < 0) {
		noct_error(env, "Cannot determine NWT1 file size."); goto cleanup;
	}
	if (!hash_file(handle->file, 0, (uint64_t)file_size_long, digest)) {
		noct_error(env, "Cannot hash NWT1 file."); goto cleanup;
	}
	noct_sha256_hex(digest, actual);
	if (strcmp(actual, expected) != 0) {
		noct_error(env, "NWT1 pack SHA-256 mismatch."); goto cleanup;
	}
	if (!parse_nwt1(env, handle, (uint64_t)file_size_long)) goto cleanup;
	if (!noct_make_empty_dict(env, &ret) ||
	    !noct_set_dict_native_pointer(env, &ret, handle, weights_finalizer))
		goto cleanup;
	installed = true;
	if (!om_freeze_dict(env, &ret)) goto cleanup;
	if (!noct_set_return(env, &ret)) goto cleanup;
	ok = true;
cleanup:
	if (!ok && handle != NULL) {
		if (!installed || noct_set_dict_native_pointer(env, &ret, NULL, NULL))
			weights_finalizer(handle);
	}
	(void)noct_unpin_local(env, 3, &path_value, &expected_value, &ret);
	return ok;
}

static bool
get_index_arg(NoctEnv *env, NoctValue *value, size_t *index)
{
	if (!noct_get_arg(env, 1, value)) return false;
	if (value->type == NOCT_VALUE_INT && value->val.i >= 0)
		*index = (size_t)(uint32_t)value->val.i;
	else if (value->type == NOCT_VALUE_LONG && value->val.l >= 0 &&
		 (uint64_t)value->val.l <= (uint64_t)SIZE_MAX)
		*index = (size_t)value->val.l;
	else {
		noct_error(env, "Weights entry index is out-of-range.");
		return false;
	}
	return true;
}

static bool
shape_matches(NoctEnv *env, NoctValue *shape, const struct nwt1_entry *entry)
{
	NoctValue dimension;
	size_t rank, i;
	uint64_t value;
	if (!noct_get_array_size(env, shape, &rank)) return false;
	if (rank != entry->rank) return false;
	for (i = 0; i < rank; i++) {
		if (!noct_get_array_elem(env, shape, i, &dimension)) return false;
		if (dimension.type == NOCT_VALUE_INT && dimension.val.i > 0)
			value = (uint64_t)(uint32_t)dimension.val.i;
		else if (dimension.type == NOCT_VALUE_LONG && dimension.val.l > 0)
			value = (uint64_t)dimension.val.l;
		else return false;
		if (value != entry->dimension[i]) return false;
	}
	return true;
}

static bool
cfunc_Weights_loadFloat32(NoctEnv *env)
{
	NoctValue handle_value, index_value, name_value, shape_value, ret;
	struct weights_handle *handle;
	struct nwt1_entry *entry;
	const char *name;
	size_t index, count, i;
	uint8_t encoded[4];
	uint32_t bits;
	float *decoded;
	bool ok;

	decoded = NULL; ok = false;
	if (!noct_pin_local(env, 5, &handle_value, &index_value, &name_value,
			    &shape_value, &ret)) return false;
	if (!noct_get_arg_check_dict(env, 0, &handle_value) ||
	    !get_weights_handle(env, &handle_value, &handle, false) ||
	    !get_index_arg(env, &index_value, &index) ||
	    !noct_get_arg_check_string(env, 2, &name_value, &name) ||
	    !noct_get_arg_check_array(env, 3, &shape_value)) goto cleanup;
	if (index >= handle->entry_count) {
		noct_error(env, "Weights entry index is out-of-range."); goto cleanup;
	}
	entry = &handle->entry[index];
	if (strcmp(name, entry->name) != 0) {
		noct_error(env, "Weights entry name mismatch."); goto cleanup;
	}
	if (!shape_matches(env, &shape_value, entry)) {
		noct_error(env, "Weights entry shape mismatch."); goto cleanup;
	}
	count = (size_t)(entry->byte_length / 4);
	if (!noct_make_packed(env, &ret, NOCT_PACKED_FLOAT32,
			      (size_t)entry->byte_length, count,
			      NULL, NULL, NULL) ||
	    !noct_get_packed_pointer(env, &ret, (void **)&decoded)) goto cleanup;
	if (!seek_file(handle->file, handle->payload_start + entry->payload_offset)) {
		noct_error(env, "Weights payload seek failed."); goto cleanup;
	}
	for (i = 0; i < count; i++) {
		if (!read_exact(handle->file, encoded, 4)) {
			noct_error(env, "Weights payload read failed."); goto cleanup;
		}
		bits = read_u32le(encoded);
		memcpy(&decoded[i], &bits, 4);
	}
	if (!noct_set_return(env, &ret)) goto cleanup;
	ok = true;
cleanup:
	(void)noct_unpin_local(env, 5, &handle_value, &index_value, &name_value,
			       &shape_value, &ret);
	return ok;
}

static bool
cfunc_Weights_close(NoctEnv *env)
{
	NoctValue handle_value, ret;
	struct weights_handle *handle;
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 2, &handle_value, &ret)) return false;
	if (!noct_get_arg_check_dict(env, 0, &handle_value) ||
	    !get_weights_handle(env, &handle_value, &handle, true)) goto cleanup;
	weights_close_contents(handle);
	if (!noct_set_return_make_int(env, &ret, 0)) goto cleanup;
	ok = true;
cleanup:
	(void)noct_unpin_local(env, 2, &handle_value, &ret);
	return ok;
}

bool
noct_register_api_weights(NoctEnv *env)
{
	static const char *open_param[] = {"path", "expectedPackSha256"};
	static const char *load_param[] = {"handle", "entryIndex", "expectedName", "expectedShape"};
	static const char *close_param[] = {"handle"};
	NoctValue package, function;
	if (!noct_make_empty_dict(env, &package) ||
	    !noct_register_cfunc(env, "Weights.open", 2, open_param,
				 cfunc_Weights_open, NULL) ||
	    !noct_get_global(env, "Weights.open", &function) ||
	    !noct_set_dict_elem_cstr(env, &package, "open", &function) ||
	    !noct_register_cfunc(env, "Weights.loadFloat32", 4, load_param,
				 cfunc_Weights_loadFloat32, NULL) ||
	    !noct_get_global(env, "Weights.loadFloat32", &function) ||
	    !noct_set_dict_elem_cstr(env, &package, "loadFloat32", &function) ||
	    !noct_register_cfunc(env, "Weights.close", 1, close_param,
				 cfunc_Weights_close, NULL) ||
	    !noct_get_global(env, "Weights.close", &function) ||
	    !noct_set_dict_elem_cstr(env, &package, "close", &function) ||
	    !om_freeze_dict(env, &package) ||
	    !noct_set_global(env, "Weights", &package)) return false;
	return true;
}
