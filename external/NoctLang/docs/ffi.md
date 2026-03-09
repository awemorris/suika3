FFI API
=======

Noct can be easily incorporated into your application. You can call
Noct scripts from your application, and Noct scripts can call back
into your application through FFI functions.

Unlike Lua's stack-based virtual machine, Noct uses a straightforward
register-based architecture, enabling more efficient JIT compilation.

The following is an example skeleton to embed Noct in your app. It
shows how to create and call a Noct virtual machine, and how to
implement an FFI function.

```
#include <noct/noct.h>

NoctVM *vm;
NoctEnv *env;

static bool ffi_print(NoctEnv *env);

void call_noct(const char *file_name, const char *file_text)
{
    // Create a VM with an environment.
    // The environment is the default thread handle.
    // All operations exclude VM destruction are done through a thread handle.
    noct_create_vm(&vm, &env);

    // Install an FFI function "print(msg)" to the VM.
    const char *param_name = {"msg"};
    noct_register_cfunc(env, "print", 1, param_name, ffi_print, NULL);

    // Compile source and install it to the VM.
    noct_register_source(env, file_name, file_text);

    // Call the main() function with no arguments.
    NoctValue ret = NOCT_ZERO;
    noct_enter_vm(env, "main", 0, NULL, &ret);

    // Destroy the runtime.
    noct_destroy_vm(env);
}

// print(msg)
static bool ffi_print(NoctEnv *env)
{
    NoctValue param;
    const char *s;

    // Pin a value to avoid GC.
    if (!noct_pin_local(env, 1, &param))
        return false;

    // Get the parameter.
    if (!noct_get_arg_check_string(env, 0, &param, &s)) {
        noct_unpin_local(env, 1, &value);
        return false;
    }

    printf("%s\n", s);

    // Unpin a value.
    noct_unpin_local(env, 1, &value);

    return true;
}
```

## Types

### NoctVM *

The type `NoctVM *` indicates a virtual machine instance.

### NoctEnv *

The type `NoctEnv *` indicates a thread instance of a virtual machine.
Most operations to virtual machines are done through `NoctEnv *` handles.

### NoctValue

The type `NoctValue` represents a value in Noct.

A value has a type and it is one of the following:

```
enum noct_value_type {
	NOCT_VALUE_INT    = 0,
	NOCT_VALUE_FLOAT  = 1,
	NOCT_VALUE_STRING = 2,
	NOCT_VALUE_ARRAY  = 3,
	NOCT_VALUE_DICT   = 4,
	NOCT_VALUE_FUNC   = 5,
};
```

A value may be initialized by the `NOCT_ZERO` macro.

```
NoctValue v = NOCT_ZERO; // NOCT_VALUE_INT, 0
```

### NoctFunc *

The type `NoctFunc` indicates a function in a virtual machine.

Using this value may save time to look up a function.

## Memory Allocation

The following macros can be defined at compilation time to change
memory allocators.

Note that they are not variables because indirect calls may be very
slow on Linux due to the Spectre vulnerability mitigation.

```
#define noct_malloc	malloc
#define noct_calloc	calloc
#define noct_strdup	strdup
#define noct_free	free
```

## Core Functions

### noct_create_vm()

This API creates a new virtual mechine instance.

It also returns a pointer to an environment for the calling thread.
Note that an environment is a per-thread handle for API calls.

```
bool
noct_create_vm(
	NoctVM **vm,
	NoctEnv **default_env);
```

### noct_destroy_vm()

This API destroys the given virtual machine instance and releases
associated resources including all environments.

```
bool
noct_destroy_vm(
	NoctVM *vm);
```

### noct_create_thread_env()

This API creates a thread-local environment for the current thread.

```
bool
noct_create_thread_env(
	NoctEnv *prev_evm,
	NoctEnv **new_env);
```

### noct_register_source()

This API registers functions from a source code text.

The given source code will be compiled and translated to machine code.

```
bool
noct_register_source(
	NoctEnv *env,
	const char *file_name,
	const char *source_text);
```

### noct_register_bytecode()

This API registers functions from compiled bytecode data.

The given bytecode will be translated to machine code.

```
bool
noct_register_bytecode(
	NoctEnv *env,
	uint8_t *data,
	size_t size);
```

### noct_register_cfunc()

This API registers an FFI function.

```
bool
noct_register_cfunc(
	NoctEnv *env,
	const char *name,
	int param_count,
	const char *param_name[],
	bool (*cfunc)(NoctEnv *env),
	NoctFunc **ret_func);
```

### noct_enter_vm()

This API enters the virtual machine in the current thread and invokes
a function by name.

It executes the specified function with the given arguments and stores
the results in the provided return value slot.

```
bool
noct_enter_vm(
	NoctEnv *env,
	const char *func_name,
	int arg_count,
	NoctValue *arg,
	NoctValue *ret);
```

### noct_get_error_file()

This API retrieves the file name where the last error occurred.

```
bool
noct_get_error_file(
	NoctEnv *env,
	const char **msg);
```

### noct_get_error_line()

This API retrieves the line number where the last error occurred.

```
bool
noct_get_error_line(
	NoctEnv *env,
	int *line);
```

### noct_get_error_message()

This API retrieves the error message where the last error occurred.

```
bool
noct_get_error_message(
	NoctEnv *env,
	const char **msg);
```

### noct_call()

This API calls a function.

While `noct_enter_vm()` receives a function name, `noct_call()`
receives a function object.

```
bool
noct_call(
	NoctEnv *env,
	NoctFunc *func,
	int arg_count,
	NoctValue *arg,
	NoctValue *ret);
```

### noct_make_int()

This API makes an integer value.

```
bool
noct_make_int(
	NoctEnv *env,
	NoctValue *val,
	int i);
```

### noct_make_float()

This API makes a float value.

```
bool
noct_make_float(
	NoctEnv *env,
	NoctValue *val,
	float f);
```

### noct_make_string()

This API makes a string value.

```
bool
noct_make_string(
	NoctEnv *env,
	NoctValue *val,
	const char *data);
```

### noct_make_string_format()

This API makes a string value with a format.

```
bool
noct_make_string_format(
	NoctEnv *env,
	NoctValue *val,
	const char *s,
	...);
```

### noct_make_empty_array()

This API makes an empty array value.

```
bool
noct_make_empty_array(
	NoctEnv *env,
	NoctValue *val);
```

### noct_make_empty_dict()

This API makes an empty dictionary value.

```
bool
noct_make_empty_dict(
	NoctEnv *env,
	NoctValue *val);
```

### noct_get_value_type()

This API retrieves the type tag of a value.

The result is one of the NOCT_VALUE_* constants.

```
bool
noct_get_value_type(
	NoctEnv *env,
	NoctValue *val,
	int *type);
```

### noct_get_int()

This API retrieves an integer from a value with type checking.

Fails if the given value is not of integer type.

```
bool
noct_get_int(
	NoctEnv *env,
	NoctValue *val,
	int *i);
```

### noct_get_float()

This API retrieves a float from a value with type checking.

Fails if the given value is not of float type.

```
bool
noct_get_float(
	NoctEnv *env,
	NoctValue *val,
	float *f);
```

### noct_get_string_len()

This API retrieves the length of the string in a value with type checking.

Fails if the given value is not of string type.

```
bool
noct_get_string_len(
	NoctEnv *env,
	NoctValue *val,
	size_t *len);
```

### noct_get_string()

This API retrieves the string from a value with type checking.

Fails if the given value is not of string type.

```
bool
noct_get_string(
	NoctEnv *env,
	NoctValue *val,
	const char **s);
```

### noct_get_func()

This API retrieves the function from a value with type checking.

Fails if the given value is not of function type.

```
bool
noct_get_func(
	NoctEnv *env,
	NoctValue *val,
	NoctFunc **ret);
```

### noct_get_array_size()

This API retrieves the number of elements in an array.

Fails if the given value is not an array.

```
bool
noct_get_array_size(
	NoctEnv *env,
	NoctValue *val,
	int *size);
```

### noct_get_array_elem()

This API retrieves an element from an array without type checking.

The element at the given index is returned as a NoctValue,
regardless of its actual type.

```
bool
noct_get_array_elem(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val);
```

### noct_set_array_elem()

This API sets an element in an array at the specified index.

The value is passed as a NoctValue and may be of any type.
If the index is out of bounds, the array is automatically expanded.
The existing element at the index, if any, will be overwritten.

```
bool
noct_set_array_elem(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val);
```

### noct_resize_array()

This API resizes an array to the specified size.

If the array is shrunk, elements beyond the new size are removed.
If the array is expanded, new elements are initialized to integer zero.

```
bool
noct_resize_array(
	NoctEnv *env,
	NoctValue *arr,
	int size);
```

### noct_make_array_copy()

This API creates a shallow copy of an array.

```
bool
noct_make_array_copy(
	NoctEnv *env,
	NoctValue *dst,
	NoctValue *src);
```

### noct_get_dict_size()

This API retrieves the number of key-value pairs in a dictionary.

```
bool
noct_get_dict_size(
	NoctEnv *env,
	NoctValue *dict,
	int *size);
```

### noct_get_dict_key_by_index()

This API retrieves a dictionary key by index.

This function can be used to traverse dictionary entries in order.

```
bool
noct_get_dict_key_by_index(
	NoctEnv *env,
	NoctValue *dict,
	int index,
	NoctValue *key);
```

### noct_get_dict_value_by_index()

This API retrieves a dictionary value by index.

This function can be used to traverse dictionary entries in order.

```
bool
noct_get_dict_value_by_index(
	NoctEnv *env,
	NoctValue *dict,
	int index,
	NoctValue *val);
```

### noct_check_dict_key()

This API checks whether a key exists in a dictionary.

```
bool
noct_check_dict_key(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	bool *ret);
```

### noct_get_dict_elem()

This API retrieves a value associated with a key in a dictionary without type checking.

The returned value is wrapped by NoctValue and may be of any type.
Fails if the key does not exist.

```
bool
noct_get_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);
```

### noct_set_dict_elem()

This API sets a value for a key in a dictionary.

```
bool
noct_set_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);
```

### noct_remove_dict_elem()

This API removes a key-value pair from a dictionary by key.

Note: this is not thread-safe and not defined in the multithreaded build.

```
bool
noct_remove_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key);
```

### noct_make_dict_copy()

This API creates a shallow copy of a dictionary.

```
bool
noct_make_dict_copy(
	NoctEnv *env,
	NoctValue *dst,
	NoctValue *src);
```

### noct_get_arg()

This API retrieves a function argument from the current stack frame.

```
bool
noct_get_arg(
	NoctEnv *env,
	int index,
	NoctValue *val);
```

### noct_set_return()

This API sets the return value for the current stack frame.

```
bool
noct_set_return(
	NoctEnv *env,
	NoctValue *val);
```

### noct_get_global()

This API retrieves the value of a global variable by name.

Fails if the global variable with the given name does not exist.

```
bool
noct_get_global(
	NoctEnv *env,
	const char *name,
	NoctValue *val);
```

### noct_set_global()

This API sets a global variable by name.

```
bool
noct_set_global(
	NoctEnv *env,
	const char *name,
	NoctValue *val);
```

### noct_pin_global()

This API declares a native global variable for use within an FFI function.

This informs the GC that the given NoctValue variable is
in use and should not be collected during GC.

```
bool
noct_pin_global(
	NoctEnv *env,
	NoctValue *val);
```

### noct_unpin_global()

This API undeclare a native global variable for use within an FFI function.

```
bool
noct_unpin_global(
	NoctEnv *env,
	NoctValue *val);
```

### noct_pin_local()

This API declares native local variables for use within an FFI function.

This informs the GC that the given stack-local NoctValue variables
are in use and should not be collected during GC. Returning from an
FFI function undeclares the local variables in the current stack.

This function should be called with the number of variables,
followed by that many NoctValue* arguments.

```
bool
noct_pin_local(
	NoctEnv *env,
	int count,
	...);
```

### noct_unpin_local()

This API undeclare a native local variables for use within an FFI function.

```
bool
noct_unpin_local(
	NoctEnv *env,
	int count,
	...);
```

### noct_fast_gc()

This API triggers a fast garbage collection pass, typically during a
periodic pause such as VSync.

This performs a collection of the young generation only.

```
void
noct_fast_gc(
	NoctEnv *env);
```

### noct_full_gc()

This API triggers a full, stop-the-world garbage collection.

This collects both young and old generations, but does not perform
memory compaction.

```
void
noct_full_gc(
	NoctEnv *env);
```

### noct_compact_gc()

This API triggers a full garbage collection followed by memory compaction.

This pass eliminates fragmentation by sliding live objects to
create a contiguous free space in the old generation.

```
void
noct_compact_gc(
	NoctEnv *env);
```

### noct_get_heap_usage()

This API retrieves the current heap usage, in bytes.

```
bool
noct_get_heap_usage(
	NoctEnv *env,
	size_t *ret);
```

### noct_error()

This API writes an error message to the internal buffer.

The message can be retrieved later using noct_get_error_message().

```
void
noct_error(
	NoctEnv *env,
	const char *msg,
	...);
```

## Convenience Functions

### noct_get_array_elem_check_int()

Convenience function to retrieve an integer element from an array
with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the element at the given index is not of integer type.

```
bool
noct_get_array_elem_check_int(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	int *i);
```

### noct_get_array_elem_check_float()

Convenience function to retrieve a float element from an array with
type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the element at the given index is not of float type.

```
bool
noct_get_array_elem_check_float(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	float *f);
```

### noct_get_array_elem_check_string()

Convenience function to retrieve a string element from an array
with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the element at the given index is not of string type.

```
bool
noct_get_array_elem_check_string(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	const char **data);
```

### noct_get_array_elem_check_array()

Convenience function to retrieve an array element from an array
with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the element at the given index is not of array type.

```
bool
noct_get_array_elem_check_array(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val);
```

### noct_get_array_elem_check_dict()

Convenience function to retrieve an array element from a dictionary
with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the element at the given index is not of dictionary type.

```
bool
noct_get_array_elem_check_dict(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val);
```

### noct_get_array_elem_check_func()

Convenience function to retrieve a function element from a
dictionary with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the element at the given index is not of function type.

```
bool
noct_get_array_elem_check_func(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	NoctFunc **f);
```

### noct_set_array_elem_make_int()

Convenience function to set an integer element in an array.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Semantically equivalent to wrapping the integer in a NoctValue
and calling noct_set_array_elem().

```
bool
noct_set_array_elem_make_int(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	int i);
```

### noct_set_array_elem_make_float()

Convenience function to set a float element in an array.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Semantically equivalent to wrapping the float in a NoctValue
and calling noct_set_array_elem().

```
bool
noct_set_array_elem_make_float(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	float f);
```

### noct_set_array_elem_make_string()

Convenience function to set a string element in an array.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Semantically equivalent to creating a string NoctValue with
noct_make_string() and passing it to noct_set_array_elem().

```
bool
noct_set_array_elem_make_string(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	const char *data);
```

### noct_get_dict_elem_check_int()

Convenience function to retrieve an integer value associated with a
key in a dictionary with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the key does not exist.
Fails if the associated value is not of integer type.

```
bool
noct_get_dict_elem_check_int(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	int *i);
```

### noct_get_dict_elem_check_float()

Convenience function to retrieve a float value associated with a
key in a dictionary with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the key does not exist.
Fails if the associated value is not of float type.

```
bool
noct_get_dict_elem_check_float(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	float *f);
```

### noct_get_dict_elem_check_string()

Convenience function to retrieve a string value associated with a
key in a dictionary with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the key does not exist.
Fails if the associated value is not of string type.

```
bool
noct_get_dict_elem_check_string(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	const char **data);
```

### noct_get_dict_elem_check_array()

Convenience function to retrieve an array value associated with a
key in a dictionary with type checking.

Fails if the key does not exist.
Fails if the associated value is not of array type.

```
bool
noct_get_dict_elem_check_array(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);
```

### noct_get_dict_elem_check_dict()

Convenience function to retrieve a dictionary value associated with a
key in a dictionary with type checking.

Fails if the key does not exist.
Fails if the associated value is not of dictionary type.

```
bool
noct_get_dict_elem_check_dict(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);
```

### noct_get_dict_elem_check_func()

Convenience function to retrieve a function value associated with a
key in a dictionary with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

Fails if the key does not exist.
Fails if the associated value is not of function type.

```
bool
noct_get_dict_elem_check_func(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	NoctFunc **f);
```

### noct_set_dict_elem_make_int()

Convenience function to set an integer value for a key in a dictionary.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_set_dict_elem_make_int(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	int i);
```

### noct_set_dict_elem_make_float()

Convenience function to set a float value for a key in a dictionary.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_set_dict_elem_make_float(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	float f);
```

### noct_set_dict_elem_make_string()

Convenience function to set a string value for a key in a dictionary.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_set_dict_elem_make_string(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	const char *data);
```

### noct_get_arg_check_int()

Convenience function to retrieve an integer function argument with
type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_get_arg_check_int(
	NoctEnv *env,
	int index,
	NoctValue *val,
	int *i);
```

### noct_get_arg_check_float()

Convenience function to retrieve a float function argument with
type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_get_arg_check_float(
	NoctEnv *env,
	int index,
	NoctValue *val,
	float *f);
```

### noct_get_arg_check_string()

Convenience function to retrieve a string function argument with
type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_get_arg_check_string(
	NoctEnv *env,
	int index,
	NoctValue *val,
	const char **data);
```

### noct_get_arg_check_array()

Convenience function to retrieve an array function argument with
type checking.

```
bool
noct_get_arg_check_array(
	NoctEnv *env,
	int index,
	NoctValue *val);
```

### noct_get_arg_check_dict()

Convenience function to retrieve a dictionary function argument with
type checking.

```
bool
noct_get_arg_check_dict(
	NoctEnv *env,
	int index,
	NoctValue *val);
```

### noct_get_arg_check_func()

Convenience function to retrieve a function argument of function
type with type checking.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_get_arg_check_func(
	NoctEnv *env,
	int index,
	NoctValue *val,
	NoctFunc **f);
```

### noct_set_return_make_int()

Convenience function to set an integer return value for the current
stack frame.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_set_return_make_int(
	NoctEnv *env,
	NoctValue *val,
	int i);
```

### noct_set_return_make_float()

Convenience function to set a float return value for the current
stack frame.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_set_return_make_float(
	NoctEnv *env,
	NoctValue *val,
	float f);
```

### noct_set_return_make_string()

Convenience function to set a string return value for the current
stack frame.

The `backing_val` parameter must point to a variable previously
declared via `noct_declare_c_local()`. This variable serves as a
pinned storage location: during the call, it temporarily holds the
element's value so that, even if a parallel GC is triggered midway,
the value remains protected and stable.

```
bool
noct_set_return_make_string(
	NoctEnv *env,
	NoctValue *val,
	const char *data);
```

## Configuration

### Enable JIT

When this variable is true, JIT compilation will be enabled.

```
bool noct_conf_jit_enable = true;
```

### JIT Thresould

When JIT is enabled and the calling count of a function reached this
value, code generation will be done for the function.

```
int noct_conf_jit_threshould = 5;
```

### Optimization Level

No optimization if `0`.
Drop line info if `1`.

```
int noct_conf_optimize = 0;
```

### GC Nursery Region Size

Nursery is the first stage region.

2 MB by default.

```
size_t noct_conf_gc_nursery_size = 2 * 1024 * 1024;
```

### GC Graduate Region Size

Graduate is the second stage region.

256 KB by default.

```
size_t noct_conf_gc_graduate_size = 256 * 1024;
```

### GC Tenure Region Size

Tenure is the last stage region, i.e., tenure region size is the heap size.

256 MB by default.

```
extern size_t noct_conf_gc_tenure_size = 256 * 1024 * 1024;
```

### GC Large-Object-Promotion Threshold

32 KB by default.

```
extern size_t noct_conf_gc_lop_threshold = 32 * 1024;
```

### GC Tenure-Promotion Threshold

```
size_t noct_conf_gc_promotion_threshold = 2;
```
