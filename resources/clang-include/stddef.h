#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef _MSC_VER
typedef unsigned long size_t;
typedef long ptrdiff_t;
#else
typedef unsigned long long size_t;
typedef long long ptrdiff_t;
#endif

#ifndef __cplusplus
#ifndef _MSC_VER
typedef int wchar_t;
#else
typedef short wchar_t;
#endif
#endif

#if __STDC_VERSION__ >= 201112L
typedef struct {
    long long __max_align_ll;
    long double __max_align_ld;
} max_align_t;
#endif

#define offsetof(type, member) ((size_t) &(((type *)0)->member))

#endif
