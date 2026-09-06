#ifndef _STRING_H
#define _STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintptr_t */

#define memcpy(dest, src, n)       __builtin_memcpy(dest, src, n)
#define memmove(dest, src, n)      __builtin_memmove(dest, src, n)
#define memset(s, c, n)            __builtin_memset(s, c, n)
#define memcmp(s1, s2, n)          __builtin_memcmp(s1, s2, n)
#define memchr(s, c, n)            __builtin_memchr(s, c, n)

/*
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int   memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);
 */

#define strcpy(dest, src)          __builtin_strcpy(dest, src)
#define strncpy(dest, src, n)      __builtin_strncpy(dest, src, n)
#define strcat(dest, src)          __builtin_strcat(dest, src)
#define strncat(dest, src, n)      __builtin_strncat(dest, src, n)

int _stricmp(const char *s1, const char *s2);

/*
char  *strcpy(char *dest, const char *src);
char  *strncpy(char *dest, const char *src, size_t n);
char  *strcat(char *dest, const char *src);
char  *strncat(char *dest, const char *src, size_t n);
 */

#define strlen(s)                  __builtin_strlen(s)
#define strcmp(s1, s2)             __builtin_strcmp(s1, s2)
#define strncmp(s1, s2, n)         __builtin_strncmp(s1, s2, n)

/*
size_t strlen(const char *s);
int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);
 */

char *strdup(const char *s);
char *_strdup(const char *s);

#define strchr(s, c)               __builtin_strchr(s, c)
#define strrchr(s, c)              __builtin_strrchr(s, c)
#define strstr(haystack, needle)   __builtin_strstr(haystack, needle)
#define strpbrk(s, accept)         __builtin_strpbrk(s, accept)
#define strspn(s, accept)          __builtin_strspn(s, accept)
#define strcspn(s, reject)         __builtin_strcspn(s, reject)

/*
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strstr(const char *haystack, const char *needle);
char  *strpbrk(const char *s, const char *accept);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
 */

#define strcoll(s1, s2)            __builtin_strcmp(s1, s2)
#define strxfrm(dest, src, n)      __builtin_strncpy(dest, src, n)

/*
int    strcoll(const char *s1, const char *s2);
size_t strxfrm(char *dest, const char *src, size_t n);
 */

char  *strtok(char *str, const char *delim);

char  *strerror(int errnum);
int    strerror_r(int errnum, char *buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif
