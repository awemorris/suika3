#ifndef _FCNTL_H
#define _FCNTL_H

#ifdef __cplusplus
extern "C" {
#endif

#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_ACCMODE   0x0003

#define O_CREAT     0x0100
#define O_EXCL      0x0200
#define O_NOCTTY    0x0400
#define O_TRUNC     0x0800
#define O_APPEND    0x1000
#define O_NONBLOCK  0x2000
#define O_DSYNC     0x4000
#define O_BINARY    0x8000
#define _O_BINARY   0x8000
#define O_SYNC      0x101000
#define O_RSYNC     0x101000
#define O_CLOEXEC   0x80000

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4

#define FD_CLOEXEC  1

typedef unsigned int mode_t;

int open(const char *pathname, int flags, ...);
int creat(const char *pathname, mode_t mode);

int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif
