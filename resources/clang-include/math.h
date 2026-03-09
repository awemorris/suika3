#ifndef _MATH_H
#define _MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define M_PI 3.14159265

typedef float  float_t;
typedef double double_t;

#define HUGE_VAL (__builtin_huge_val())
#define HUGE_VALF (__builtin_huge_valf())
#define HUGE_VALL (__builtin_huge_vall())

#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))

#define FP_NAN         0
#define FP_INFINITE    1
#define FP_ZERO        2
#define FP_SUBNORMAL   3
#define FP_NORMAL      4

#define fpclassify(x)  __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x)

/*
#define fpclassify(x)				\
    (isnan(x) ? FP_NAN :			\
     isinf(x) ? FP_INFINITE :			\
     (x == 0.0) ? FP_ZERO :			\
     /\* note: this is a simplification! *\/	\
     FP_NORMAL)
 */

#define fabs(x)        __builtin_fabs(x)
#define fabsf(x)       __builtin_fabsf(x)
#define fabsl(x)       __builtin_fabsl(x)

/*
double fabs(double x);
float  fabsf(float x);
long double fabsl(long double x);
 */

#define sin(x)         __builtin_sin(x)
#define sinf(x)        __builtin_sinf(x)
#define sinl(x)        __builtin_sinl(x)

/*
double sin(double x);
float  sinf(float x);
long double sinl(long double x);
 */

#define cos(x)         __builtin_cos(x)
#define cosf(x)        __builtin_cosf(x)
#define cosl(x)        __builtin_cosl(x)

/*
double cos(double x);
float  cosf(float x);
long double cosl(long double x);
 */

#define tan(x)         __builtin_tan(x)
#define tanf(x)        __builtin_tanf(x)
#define tanl(x)        __builtin_tanl(x)

/*
double tan(double x);
float  tanf(float x);
long double tanl(long double x);
 */

#define atan(x)		__builtin_atan(x)
#define atan2f(x)	__builtin_atan2f(x)
#define atanf(x)	__builtin_atanf(x)

/*
double atan(double x);
float atan2f(float y, float x);
float atanf(float x);
 */
long double atanl(long double x);

#define acos(x)		__builtin_acos(x)
#define acosf(x)	__builtin_acosf(x)

/*
double acos(double x);
float acosf(float x);
 */
long double acosl(long double x);

#define exp(x)         __builtin_exp(x)
#define expf(x)        __builtin_expf(x)
#define expl(x)        __builtin_expl(x)

/*
double exp(double x);
float  expf(float x);
long double expl(long double x);
 */

#define log(x)         __builtin_log(x)
#define logf(x)        __builtin_logf(x)
#define logl(x)        __builtin_logl(x)

/*
double log(double x);
float  logf(float x);
long double logl(long double x);
 */

#define log10(x)       __builtin_log10(x)
#define log10f(x)      __builtin_log10f(x)
#define log10l(x)      __builtin_log10l(x)

/*
double log10(double x);
float  log10f(float x);
long double log10l(long double x);
*/

#define pow(x, y)      __builtin_pow(x, y)
#define powf(x, y)     __builtin_powf(x, y)
#define powl(x, y)     __builtin_powl(x, y)

/*
double pow(double x, double y);
float  powf(float x, float y);
long double powl(long double x, long double y);
*/

#define sqrt(x)        __builtin_sqrt(x)
#define sqrtf(x)       __builtin_sqrtf(x)
#define sqrtl(x)       __builtin_sqrtl(x)

/*
double sqrt(double x);
float  sqrtf(float x);
long double sqrtl(long double x);
*/

#define floor(x)       __builtin_floor(x)
#define ceil(x)        __builtin_ceil(x)
#define trunc(x)       __builtin_trunc(x)
#define round(x)       __builtin_round(x)

/*
double floor(double x);
double ceil(double x);
double trunc(double x);
double round(double x);
 */

#define floorf(x)      __builtin_floorf(x)
#define ceilf(x)       __builtin_ceilf(x)
#define truncf(x)      __builtin_truncf(x)
#define roundf(x)      __builtin_roundf(x)

/*
float  floorf(float x);
float  ceilf(float x);
float  truncf(float x);
float  roundf(float x);
*/

long double floorl(long double x);
long double ceill(long double x);
long double truncl(long double x);
long double roundl(long double x);

#define fmin(x, y)     __builtin_fmin(x, y)
#define fmax(x, y)     __builtin_fmax(x, y)
#define fminf(x, y)    __builtin_fminf(x, y)
#define fmaxf(x, y)    __builtin_fmaxf(x, y)

/*
double fmin(double x, double y);
double fmax(double x, double y);
float  fminf(float x, float y);
float  fmaxf(float x, float y);
 */

long double fminl(long double x, long double y);
long double fmaxl(long double x, long double y);

#define isnan(x)       __builtin_isnan(x)
#define isinf(x)       __builtin_isinf(x)
#define isfinite(x)    __builtin_isfinite(x)

/*
int isnan(double x);
int isinf(double x);
int isfinite(double x);
 */

#define frexp(x,y)     __builtin_frexp(x,y)
#define modf(x,y)      __builtin_modf(x,y)
#define frexpf(x,y)    __builtin_frexpf(x,y)
#define modff(x,y)     __builtin_modff(x,y)

/*
double frexp(double value, int *exp);
double modf(double value, double *iptr);
float frexpf(float value, int *exp);
float modff(float value, float *iptr);
*/
long double frexpl(long double value, int *exp);
long double modfl(long double value, long double *iptr);

#define ldexp(x,e)	__builtin_ldexp(x,e)
#define ldexpf(x,e)	__builtin_ldexpf(x,e)

/*
double ldexp(double x, int exp);
float ldexpf(float x, int exp);
*/
long double ldexpl(long double x, int exp);

#define rint(x)		__builtin_rint(x)
#define rintf(x)	__builtin_rintf(x)

/*
double rint(double x);
float rintf(float x);
*/
long double rintl(long double x);

#ifdef __cplusplus
}
#endif

#endif
