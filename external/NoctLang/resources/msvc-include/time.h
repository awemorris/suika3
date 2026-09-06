#ifndef _TIME_H
#define _TIME_H

#include <stddef.h> /* NULL, size_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef long time_t;
typedef long clock_t;
typedef int clockid_t;
typedef int timer_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

time_t time(time_t *t);
double difftime(time_t end, time_t start);
time_t mktime(struct tm *tm);
char *asctime(const struct tm *tm);
char *ctime(const time_t *timep);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);

#define CLOCKS_PER_SEC  1000000L

#ifdef __cplusplus
}
#endif

#endif
