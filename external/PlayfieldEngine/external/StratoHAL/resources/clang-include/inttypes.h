#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>
#include <stddef.h> /* wchar_t */

#ifdef __cplusplus
extern "C" {
#endif

#define PRId8       "hhd"
#define PRIi8       "hhi"
#define PRIo8       "hho"
#define PRIu8       "hhu"
#define PRIx8       "hhx"
#define PRIX8       "hhX"

#define PRId16      "hd"
#define PRIi16      "hi"
#define PRIo16      "ho"
#define PRIu16      "hu"
#define PRIx16      "hx"
#define PRIX16      "hX"

#define PRId32      "d"
#define PRIi32      "i"
#define PRIo32      "o"
#define PRIu32      "u"
#define PRIx32      "x"
#define PRIX32      "X"

#define PRId64      "ld"
#define PRIi64      "li"
#define PRIo64      "lo"
#define PRIu64      "lu"
#define PRIx64      "lx"
#define PRIX64      "lX"

#define SCNd8       "hhd"
#define SCNi8       "hhi"
#define SCNo8       "hho"
#define SCNu8       "hhu"
#define SCNx8       "hhx"

#define SCNd16      "hd"
#define SCNi16      "hi"
#define SCNo16      "ho"
#define SCNu16      "hu"
#define SCNx16      "hx"

#define SCNd32      "d"
#define SCNi32      "i"
#define SCNo32      "o"
#define SCNu32      "u"
#define SCNx32      "x"

#define SCNd64      "ld"
#define SCNi64      "li"
#define SCNo64      "lo"
#define SCNu64      "lu"
#define SCNx64      "lx"

#define PRIdPTR     "ld"
#define PRIiPTR     "li"
#define PRIoPTR     "lo"
#define PRIuPTR     "lu"
#define PRIxPTR     "lx"
#define PRIXPTR     "lX"

#define SCNdPTR     "ld"
#define SCNiPTR     "li"
#define SCNoPTR     "lo"
#define SCNuPTR     "lu"
#define SCNxPTR     "lx"

intmax_t strtoimax(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);

intmax_t wcstoimax(const wchar_t *nptr, wchar_t **endptr, int base);
uintmax_t wcstoumax(const wchar_t *nptr, wchar_t **endptr, int base);

#ifdef __cplusplus
}
#endif

#endif
