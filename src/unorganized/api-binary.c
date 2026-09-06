/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Checked Binary.* and Hash.* APIs used by the ONNX converter.
 */

#include <noct/noct.h>
#include "sha256.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct binary_ffi_item {
	const char *global_name;
	const char *package_name;
	const char *field_name;
	size_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};

static bool cfunc_Binary_readVarintUnsigned(NoctEnv *env);
static bool cfunc_Binary_readVarintSigned(NoctEnv *env);
static bool cfunc_Binary_skipVarint(NoctEnv *env);
static bool cfunc_Binary_readU32LE(NoctEnv *env);
static bool cfunc_Binary_readI64LE(NoctEnv *env);
static bool cfunc_Binary_writeU32LE(NoctEnv *env);
static bool cfunc_Binary_writeU64LE(NoctEnv *env);
static bool cfunc_Binary_readFloat32LE(NoctEnv *env);
static bool cfunc_Binary_writeFloat32LE(NoctEnv *env);
static bool cfunc_Hash_sha256(NoctEnv *env);
static bool cfunc_Hash_sha256Bytes(NoctEnv *env);

static struct binary_ffi_item binary_ffi_items[] = {
	{"Binary.readVarintUnsigned", "Binary", "readVarintUnsigned", 3,
	 {"bytes", "offset", "limit"}, cfunc_Binary_readVarintUnsigned},
	{"Binary.readVarintSigned", "Binary", "readVarintSigned", 3,
	 {"bytes", "offset", "limit"}, cfunc_Binary_readVarintSigned},
	{"Binary.skipVarint", "Binary", "skipVarint", 3,
	 {"bytes", "offset", "limit"}, cfunc_Binary_skipVarint},
	{"Binary.readU32LE", "Binary", "readU32LE", 2,
	 {"bytes", "offset"}, cfunc_Binary_readU32LE},
	{"Binary.readI64LE", "Binary", "readI64LE", 2,
	 {"bytes", "offset"}, cfunc_Binary_readI64LE},
	{"Binary.writeU32LE", "Binary", "writeU32LE", 3,
	 {"bytes", "offset", "value"}, cfunc_Binary_writeU32LE},
	{"Binary.writeU64LE", "Binary", "writeU64LE", 3,
	 {"bytes", "offset", "value"}, cfunc_Binary_writeU64LE},
	{"Binary.readFloat32LE", "Binary", "readFloat32LE", 2,
	 {"path", "elementCount"}, cfunc_Binary_readFloat32LE},
	{"Binary.writeFloat32LE", "Binary", "writeFloat32LE", 2,
	 {"path", "packed"}, cfunc_Binary_writeFloat32LE},
	{"Hash.sha256", "Hash", "sha256", 1,
	 {"bytes"}, cfunc_Hash_sha256},
	{"Hash.sha256Bytes", "Hash", "sha256Bytes", 1,
	 {"bytes"}, cfunc_Hash_sha256Bytes}
};

static bool
get_nonnegative_arg(NoctEnv *env, uint32_t index, NoctValue *value,
		    size_t *result)
{
	uint64_t u;
	if (!noct_get_arg(env, index, value)) return false;
	if (value->type == NOCT_VALUE_INT) {
		if (value->val.i < 0) {
			noct_error(env, "Binary size/offset argument must be non-negative.");
			return false;
		}
		u = (uint64_t)(uint32_t)value->val.i;
	} else if (value->type == NOCT_VALUE_LONG) {
		if (value->val.l < 0) {
			noct_error(env, "Binary size/offset argument must be non-negative.");
			return false;
		}
		u = (uint64_t)value->val.l;
	} else {
		noct_error(env, "Binary size/offset argument is not an integer.");
		return false;
	}
	if (u > (uint64_t)SIZE_MAX) {
		noct_error(env, "Binary size/offset argument is too large.");
		return false;
	}
	*result = (size_t)u;
	return true;
}

static bool
get_integer_bits(NoctEnv *env, uint32_t index, NoctValue *value,
		 uint64_t *result)
{
	if (!noct_get_arg(env, index, value)) return false;
	if (value->type == NOCT_VALUE_INT)
		*result = (uint64_t)(int64_t)value->val.i;
	else if (value->type == NOCT_VALUE_LONG)
		*result = (uint64_t)value->val.l;
	else {
		noct_error(env, "Binary value argument is not an integer.");
		return false;
	}
	return true;
}

static bool
get_bytes(NoctEnv *env, uint32_t index, NoctValue *value,
	  uint8_t **bytes, size_t *size)
{
	void *pointer;
	if (!noct_get_arg_check_packed(env, index, value, NOCT_PACKED_UINT8) ||
	    !noct_get_packed_size(env, value, size) ||
	    !noct_get_packed_pointer(env, value, &pointer))
		return false;
	*bytes = (uint8_t *)pointer;
	return true;
}

static bool
parse_varint(NoctEnv *env, const uint8_t *bytes, size_t offset,
	     size_t limit, uint64_t *value, size_t *next)
{
	uint64_t result;
	uint8_t byte;
	int i;

	result = 0;
	for (i = 0; i < 10; i++) {
		if (offset >= limit) {
			noct_error(env, "Truncated varint.");
			return false;
		}
		byte = bytes[offset++];
		if (i == 9 && (byte & 0xfeU) != 0) {
			noct_error(env, "Varint exceeds 64 bits.");
			return false;
		}
		result |= (uint64_t)(byte & 0x7fU) << (i * 7);
		if ((byte & 0x80U) == 0) {
			if (i != 0 && byte == 0) {
				noct_error(env, "Overlong varint encoding.");
				return false;
			}
			*value = result;
			*next = offset;
			return true;
		}
	}
	noct_error(env, "Varint exceeds ten bytes.");
	return false;
}

static bool
get_varint_args(NoctEnv *env, NoctValue *bytes_value,
		NoctValue *offset_value, NoctValue *limit_value,
		uint8_t **bytes, size_t *offset, size_t *limit)
{
	size_t size;
	if (!get_bytes(env, 0, bytes_value, bytes, &size) ||
	    !get_nonnegative_arg(env, 1, offset_value, offset) ||
	    !get_nonnegative_arg(env, 2, limit_value, limit))
		return false;
	if (*limit > size || *offset > *limit) {
		noct_error(env, "Binary cursor range is out-of-bounds.");
		return false;
	}
	return true;
}

static bool
cfunc_Binary_readVarintUnsigned(NoctEnv *env)
{
	NoctValue bytes_value, offset_value, limit_value, ret;
	uint8_t *bytes;
	size_t offset, limit, next;
	uint64_t value;
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 4, &bytes_value, &offset_value,
			    &limit_value, &ret)) return false;
	if (get_varint_args(env, &bytes_value, &offset_value, &limit_value,
			    &bytes, &offset, &limit) &&
	    parse_varint(env, bytes, offset, limit, &value, &next) &&
	    noct_set_return_make_long(env, &ret, (int64_t)value)) ok = true;
	(void)noct_unpin_local(env, 4, &bytes_value, &offset_value,
			       &limit_value, &ret);
	return ok;
}

static bool
cfunc_Binary_readVarintSigned(NoctEnv *env)
{
	return cfunc_Binary_readVarintUnsigned(env);
}

static bool
cfunc_Binary_skipVarint(NoctEnv *env)
{
	NoctValue bytes_value, offset_value, limit_value, ret;
	uint8_t *bytes;
	size_t offset, limit, next;
	uint64_t value;
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 4, &bytes_value, &offset_value,
			    &limit_value, &ret)) return false;
	if (get_varint_args(env, &bytes_value, &offset_value, &limit_value,
			    &bytes, &offset, &limit) &&
	    parse_varint(env, bytes, offset, limit, &value, &next) &&
	    noct_set_return_make_int_long(env, &ret, next)) ok = true;
	(void)noct_unpin_local(env, 4, &bytes_value, &offset_value,
			       &limit_value, &ret);
	return ok;
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
	uint64_t result;
	int i;
	result = 0;
	for (i = 0; i < 8; i++) result |= (uint64_t)p[i] << (i * 8);
	return result;
}

static void
write_u32le(uint8_t *p, uint32_t value)
{
	int i;
	for (i = 0; i < 4; i++) p[i] = (uint8_t)(value >> (i * 8));
}

static void
write_u64le(uint8_t *p, uint64_t value)
{
	int i;
	for (i = 0; i < 8; i++) p[i] = (uint8_t)(value >> (i * 8));
}

static bool
get_fixed_args(NoctEnv *env, NoctValue *bytes_value,
	       NoctValue *offset_value, size_t width,
	       uint8_t **bytes, size_t *offset)
{
	size_t size;
	if (!get_bytes(env, 0, bytes_value, bytes, &size) ||
	    !get_nonnegative_arg(env, 1, offset_value, offset)) return false;
	if (*offset > size || width > size - *offset) {
		noct_error(env, "Binary fixed-width access is out-of-bounds.");
		return false;
	}
	return true;
}

static bool
cfunc_Binary_readU32LE(NoctEnv *env)
{
	NoctValue bytes_value, offset_value, ret;
	uint8_t *bytes;
	size_t offset;
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 3, &bytes_value, &offset_value, &ret)) return false;
	if (get_fixed_args(env, &bytes_value, &offset_value, 4, &bytes, &offset) &&
	    noct_set_return_make_long(env, &ret,
			      (int64_t)(uint64_t)read_u32le(bytes + offset))) ok = true;
	(void)noct_unpin_local(env, 3, &bytes_value, &offset_value, &ret);
	return ok;
}

static bool
cfunc_Binary_readI64LE(NoctEnv *env)
{
	NoctValue bytes_value, offset_value, ret;
	uint8_t *bytes;
	size_t offset;
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 3, &bytes_value, &offset_value, &ret)) return false;
	if (get_fixed_args(env, &bytes_value, &offset_value, 8, &bytes, &offset) &&
	    noct_set_return_make_long(env, &ret,
			      (int64_t)read_u64le(bytes + offset))) ok = true;
	(void)noct_unpin_local(env, 3, &bytes_value, &offset_value, &ret);
	return ok;
}

static bool
cfunc_Binary_writeU32LE(NoctEnv *env)
{
	NoctValue bytes_value, offset_value, value_value, ret;
	uint8_t *bytes;
	size_t offset;
	uint64_t value;
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 4, &bytes_value, &offset_value,
			    &value_value, &ret)) return false;
	if (get_fixed_args(env, &bytes_value, &offset_value, 4, &bytes, &offset) &&
	    get_integer_bits(env, 2, &value_value, &value)) {
		if (value_value.type == NOCT_VALUE_INT)
			value = (uint64_t)(uint32_t)value_value.val.i;
		if ((uint32_t)(value >> 32) != 0 ||
		    (value_value.type == NOCT_VALUE_LONG && value_value.val.l < 0)) {
			noct_error(env, "Binary.writeU32LE value is out-of-range.");
		} else {
			write_u32le(bytes + offset, (uint32_t)value);
			if (noct_set_return_make_int(env, &ret, 0)) ok = true;
		}
	}
	(void)noct_unpin_local(env, 4, &bytes_value, &offset_value,
			       &value_value, &ret);
	return ok;
}

static bool
cfunc_Binary_writeU64LE(NoctEnv *env)
{
	NoctValue bytes_value, offset_value, value_value, ret;
	uint8_t *bytes;
	size_t offset;
	uint64_t value;
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 4, &bytes_value, &offset_value,
			    &value_value, &ret)) return false;
	if (get_fixed_args(env, &bytes_value, &offset_value, 8, &bytes, &offset) &&
	    get_integer_bits(env, 2, &value_value, &value)) {
		write_u64le(bytes + offset, value);
		if (noct_set_return_make_int(env, &ret, 0)) ok = true;
	}
	(void)noct_unpin_local(env, 4, &bytes_value, &offset_value,
			       &value_value, &ret);
	return ok;
}

static bool
cfunc_Binary_readFloat32LE(NoctEnv *env)
{
	NoctValue path_value, count_value, ret;
	const char *path;
	size_t count, bytes_size, i;
	FILE *file;
	uint8_t encoded[4];
	float *decoded;
	uint32_t bits;
	bool ok;

	file = NULL; decoded = NULL; ok = false;
	if (!noct_pin_local(env, 3, &path_value, &count_value, &ret)) return false;
	if (!noct_get_arg_check_string(env, 0, &path_value, &path) ||
	    !get_nonnegative_arg(env, 1, &count_value, &count)) goto cleanup;
	if (count == 0 || count > (size_t)INT_MAX || count > SIZE_MAX / 4) {
		noct_error(env, "Float32 element count is out-of-range.");
		goto cleanup;
	}
	bytes_size = count * 4;
	file = fopen(path, "rb");
	if (file == NULL) { noct_error(env, "Cannot open file %s.", path); goto cleanup; }
	if (!noct_make_packed(env, &ret, NOCT_PACKED_FLOAT32,
			      bytes_size, count, NULL, NULL, NULL) ||
	    !noct_get_packed_pointer(env, &ret, (void **)&decoded)) goto cleanup;
	for (i = 0; i < count; i++) {
		if (fread(encoded, 1, 4, file) != 4) {
			noct_error(env, "Float32 file is shorter than expected.");
			goto cleanup;
		}
		bits = read_u32le(encoded);
		memcpy(&decoded[i], &bits, 4);
	}
	if (fgetc(file) != EOF) {
		noct_error(env, "Float32 file has trailing bytes.");
		goto cleanup;
	}
	if (ferror(file)) { noct_error(env, "File read error."); goto cleanup; }
	if (!noct_set_return(env, &ret)) goto cleanup;
	ok = true;
cleanup:
	if (file != NULL) (void)fclose(file);
	(void)noct_unpin_local(env, 3, &path_value, &count_value, &ret);
	return ok;
}

static bool
cfunc_Binary_writeFloat32LE(NoctEnv *env)
{
	NoctValue path_value, packed_value, ret;
	const char *path;
	void *pointer;
	size_t count, i;
	FILE *file;
	uint32_t bits;
	uint8_t encoded[4];
	bool ok;

	file = NULL; ok = false;
	if (!noct_pin_local(env, 3, &path_value, &packed_value, &ret)) return false;
	if (!noct_get_arg_check_string(env, 0, &path_value, &path) ||
	    !noct_get_arg_check_packed(env, 1, &packed_value, NOCT_PACKED_FLOAT32) ||
	    !noct_get_packed_size(env, &packed_value, &count) ||
	    !noct_get_packed_pointer(env, &packed_value, &pointer)) goto cleanup;
	file = fopen(path, "wb");
	if (file == NULL) { noct_error(env, "Cannot open file %s.", path); goto cleanup; }
	for (i = 0; i < count; i++) {
		memcpy(&bits, (const float *)pointer + i, 4);
		write_u32le(encoded, bits);
		if (fwrite(encoded, 1, 4, file) != 4) {
			noct_error(env, "File write error.");
			goto cleanup;
		}
	}
	if (fflush(file) != 0) { noct_error(env, "File write error."); goto cleanup; }
	if (fclose(file) != 0) {
		file = NULL;
		noct_error(env, "File close error.");
		goto cleanup;
	}
	file = NULL;
	if (!noct_set_return_make_int(env, &ret, 0)) goto cleanup;
	ok = true;
cleanup:
	if (file != NULL) (void)fclose(file);
	(void)noct_unpin_local(env, 3, &path_value, &packed_value, &ret);
	return ok;
}

static bool
hash_arg(NoctEnv *env, NoctValue *bytes_value, uint8_t digest[32])
{
	uint8_t *bytes;
	size_t size;
	if (!get_bytes(env, 0, bytes_value, &bytes, &size)) return false;
	noct_sha256_bytes(bytes, size, digest);
	return true;
}

static bool
cfunc_Hash_sha256(NoctEnv *env)
{
	NoctValue bytes_value, ret;
	uint8_t digest[32];
	char hex[65];
	bool ok;
	ok = false;
	if (!noct_pin_local(env, 2, &bytes_value, &ret)) return false;
	if (hash_arg(env, &bytes_value, digest)) {
		noct_sha256_hex(digest, hex);
		if (noct_set_return_make_string(env, &ret, hex)) ok = true;
	}
	(void)noct_unpin_local(env, 2, &bytes_value, &ret);
	return ok;
}

static bool
cfunc_Hash_sha256Bytes(NoctEnv *env)
{
	NoctValue bytes_value, ret;
	uint8_t digest[32];
	void *copy;
	bool ok;
	copy = NULL; ok = false;
	if (!noct_pin_local(env, 2, &bytes_value, &ret)) return false;
	if (!hash_arg(env, &bytes_value, digest)) goto cleanup;
	if (!noct_make_packed(env, &ret, NOCT_PACKED_UINT8, 32, 32,
			      NULL, NULL, NULL) ||
	    !noct_get_packed_pointer(env, &ret, &copy)) goto cleanup;
	memcpy(copy, digest, 32);
	if (!noct_set_return(env, &ret)) goto cleanup;
	ok = true;
cleanup:
	(void)noct_unpin_local(env, 2, &bytes_value, &ret);
	return ok;
}

bool
noct_register_api_binary(NoctEnv *env)
{
	NoctValue binary_dict, hash_dict;
	size_t i;
	if (!noct_make_empty_dict(env, &binary_dict) ||
	    !noct_make_empty_dict(env, &hash_dict) ||
	    !noct_set_global(env, "Binary", &binary_dict) ||
	    !noct_set_global(env, "Hash", &hash_dict)) return false;
	for (i = 0; i < sizeof(binary_ffi_items) / sizeof(binary_ffi_items[0]); i++) {
		NoctValue func_value;
		NoctValue *package;
		package = strcmp(binary_ffi_items[i].package_name, "Binary") == 0 ?
			  &binary_dict : &hash_dict;
		if (!noct_register_cfunc(env, binary_ffi_items[i].global_name,
					 binary_ffi_items[i].param_count,
					 binary_ffi_items[i].param,
					 binary_ffi_items[i].cfunc, NULL) ||
		    !noct_get_global(env, binary_ffi_items[i].global_name, &func_value) ||
		    !noct_set_dict_elem_cstr(env, package,
					     binary_ffi_items[i].field_name,
					     &func_value)) return false;
	}
	return true;
}
