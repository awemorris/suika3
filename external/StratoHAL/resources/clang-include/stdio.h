#ifndef _STDIO_H
#define _STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  /* size_t */
#include <stdarg.h>  /* va_list */

#define NULL		((void *)0)
#define EOF		(-1)
#define FOPEN_MAX	1024
#define FILENAME_MAX	1024
#define BUFSIZ		1024
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

typedef struct _FILE FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

typedef struct {
	int dummy;
} fpos_t;

FILE *fopen(const char *filename, const char *mode);
FILE *freopen(const char *filename, const char *mode, FILE *stream);
int fclose(FILE *stream);
int remove(const char *filename);
int rename(const char *oldname, const char *newname);
FILE *tmpfile(void);
char *tmpnam(char *str);

int fprintf(FILE *stream, const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int printf(const char *format, ...);
int scanf(const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int sscanf(const char *str, const char *format, ...);

int vfprintf(FILE *stream, const char *format, va_list arg);
int vfscanf(FILE *stream, const char *format, va_list arg);
int vprintf(const char *format, va_list arg);
int vscanf(const char *format, va_list arg);
int vsnprintf(char *str, size_t size, const char *format, va_list arg);
int vsprintf(char *str, const char *format, va_list arg);
int vsscanf(const char *str, const char *format, va_list arg);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int fgetc(FILE *stream);
char *fgets(char *str, int n, FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *str, FILE *stream);
int getc(FILE *stream);
int getchar(void);
int putc(int c, FILE *stream);
int putchar(int c);
char *gets(char *str);
int puts(const char *str);
int ungetc(int c, FILE *stream);

int fflush(FILE *stream);
void setbuf(FILE *stream, char *buffer);
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);

long ftell(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
void rewind(FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);

void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

FILE *fdopen(int fd, const char *mode);
int fileno(FILE *stream);

extern int _fmode;

int _fseeki64(FILE *stream, long long offset, int whence);

#ifdef __cplusplus
}
#endif

#endif
