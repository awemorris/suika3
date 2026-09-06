#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stddef.h>   // for size_t, ptrdiff_t
#include <stdint.h>   // for int*_t

#ifdef __cplusplus
extern "C" {
#endif

typedef long           ssize_t;     // signed size_t
typedef unsigned long  dev_t;       // device ID
typedef unsigned long  ino_t;       // inode number
typedef unsigned int   mode_t;      // permission flags
typedef unsigned int   nlink_t;     // number of hard links
typedef unsigned int   uid_t;       // user ID
typedef unsigned int   gid_t;       // group ID
typedef long           off_t;       // file offset
typedef int            pid_t;       // process ID
typedef long           time_t;      // time (seconds since epoch)
typedef long           clock_t;     // clock ticks
typedef int            clockid_t;   // clock ID
typedef int            timer_t;     // timer ID
typedef unsigned long  blkcnt_t;    // number of blocks
typedef unsigned long  blksize_t;   // block size
typedef unsigned long  fsblkcnt_t;  // total blocks in file system
typedef unsigned long  fsfilcnt_t;  // total file nodes in file system

typedef unsigned long  useconds_t;  // microseconds
typedef long           suseconds_t; // signed microseconds

//typedef uintptr_t      intptr_t;

#ifdef __cplusplus
}
#endif

#endif
