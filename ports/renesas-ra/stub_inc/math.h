/* Complete math.h stub for freestanding ARM using compiler builtins where possible */
#ifndef _MATH_H
#define _MATH_H

#include <stddef.h>
#include <stdint.h>

#define FLT_MAX     3.40282347e+38F
#define FLT_MIN     1.17549435e-38F
#define FLT_EPSILON 1.19209290e-7F

#define DBL_MAX     1.7976931348623157e+308
#define DBL_MIN     2.2250738585072014e-308
#define DBL_EPSILON 2.2204460492503131e-16

#define NAN         (__builtin_nanf(""))
#define INFINITY    (__builtin_inff())
#define HUGE_VALF   (__builtin_inff())
#define HUGE_VAL    ((double)__builtin_inff())

/* FP classification constants */
#define FP_ZERO       0
#define FP_SUBNORMAL  1
#define FP_NORMAL     2
#define FP_INFINITE   3
#define FP_NAN        4

/* Classification macros - compiler builtins */
#define fpclassify(x) __builtin_fpclassify(0, 1, 2, 3, 4, x)
#define signbit(x) __builtin_signbit(x)
#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf(x)
#define isfinite(x) __builtin_isfinite(x)
#define isnormal(x) __builtin_isnormal(x)

/* Basic trig */
double sin(double) __attribute__((const));
double cos(double) __attribute__((const));
double tan(double) __attribute__((const));
float sinf(float) __attribute__((const));
float cosf(float) __attribute__((const));
float tanf(float) __attribute__((const));

/* Inverse trig */
double asin(double) __attribute__((const));
double acos(double) __attribute__((const));
double atan(double) __attribute__((const));
double atan2(double, double) __attribute__((const));
float asinf(float) __attribute__((const));
float acosf(float) __attribute__((const));
float atanf(float) __attribute__((const));
float atan2f(float, float) __attribute__((const));

/* Hyperbolic */
double sinh(double) __attribute__((const));
double cosh(double) __attribute__((const));
double tanh(double) __attribute__((const));
double asinh(double) __attribute__((const));
double acosh(double) __attribute__((const));
double atanh(double) __attribute__((const));
float sinhf(float) __attribute__((const));
float coshf(float) __attribute__((const));
float tanhf(float) __attribute__((const));
float asinhf(float) __attribute__((const));
float acoshf(float) __attribute__((const));
float atanhf(float) __attribute__((const));

/* Exponentials and logarithms */
double exp(double) __attribute__((const));
double log(double) __attribute__((const));
double exp2(double) __attribute__((const));
double log2(double) __attribute__((const));
double expm1(double) __attribute__((const));
double log1p(double) __attribute__((const));
double exp10(double) __attribute__((const));
double log10(double) __attribute__((const));
float expf(float) __attribute__((const));
float logf(float) __attribute__((const));
float exp2f(float) __attribute__((const));
float log2f(float) __attribute__((const));
float expm1f(float) __attribute__((const));
float log1pf(float) __attribute__((const));
float exp10f(float) __attribute__((const));
float log10f(float) __attribute__((const));

/* Powers */
double sqrt(double) __attribute__((const));
double pow(double, double) __attribute__((const));
double cbrt(double) __attribute__((const));
double hypot(double, double) __attribute__((const));
float sqrtf(float) __attribute__((const));
float powf(float, float) __attribute__((const));
float cbrtf(float) __attribute__((const));
float hypotf(float, float) __attribute__((const));

/* Error and gamma */
double erf(double) __attribute__((const));
double erfc(double) __attribute__((const));
double tgamma(double) __attribute__((const));
double lgamma(double) __attribute__((const));
float erff(float) __attribute__((const));
float erfcf(float) __attribute__((const));
float tgammaf(float) __attribute__((const));
float lgammaf(float) __attribute__((const));

/* Rounding */
double floor(double) __attribute__((const));
double ceil(double) __attribute__((const));
double round(double) __attribute__((const));
double trunc(double) __attribute__((const));
double nearbyint(double) __attribute__((const));
double rint(double) __attribute__((const));
long lrint(double) __attribute__((const));
float floorf(float) __attribute__((const));
float ceilf(float) __attribute__((const));
float roundf(float) __attribute__((const));
float truncf(float) __attribute__((const));
float nearbyintf(float) __attribute__((const));
float rintf(float) __attribute__((const));
long lrintf(float) __attribute__((const));

/* Absolute value */
double fabs(double) __attribute__((const));
float fabsf(float) __attribute__((const));

/* Remainder */
double fmod(double, double) __attribute__((const));
float fmodf(float, float) __attribute__((const));
double remainder(double, double) __attribute__((const));
float remainderf(float, float) __attribute__((const));

/* Sign manipulation */
double copysign(double, double) __attribute__((const));
float copysignf(float, float) __attribute__((const));
double nan(const char *) __attribute__((const));
float nanf(const char *) __attribute__((const));

/* Decomposition */
double frexp(double, int *) __attribute__((const));
double ldexp(double, int) __attribute__((const));
double modf(double, double *) __attribute__((const));
float frexpf(float, int *) __attribute__((const));
float ldexpf(float, int) __attribute__((const));
float modff(float, float *) __attribute__((const));

/* Scaling */
double scalbn(double, int) __attribute__((const));
double scalbln(double, long int) __attribute__((const));
float scalbnf(float, int) __attribute__((const));
float scalblnf(float, long int) __attribute__((const));

#endif /* _MATH_H */
