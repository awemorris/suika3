#ifndef _ERRNO_H
#define _ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

extern int errno;

#define EDOM        1
#define ERANGE      2
#define EINVAL      3
#define EIO         4
#define ENOENT      5
#define E2BIG       6
#define ENOMEM      7
#define EPERM       8
#define EACCES      9
#define EBADF       10
#define EAGAIN      11
#define EINTR       12
#define ENOSPC      13
#define ENOTDIR     14
#define EISDIR      15
#define EEXIST      16
#define ENOTEMPTY   17
#define EMFILE      18

#ifdef __cplusplus
}
#endif

#endif
