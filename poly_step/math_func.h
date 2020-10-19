/*
 * Provide double fallbacks for environments lacking sinf and
 * friends (e.g. Solaris)
 */

#ifndef math_func_h
#define math_func_h

#include <math.h>

#ifndef M_PI
#    define M_PI 3.14159265358979323846 /* pi */
#endif

#ifndef M_LN10
#    define M_LN10 2.30258509299404568402  /* log_e(10) */
#endif

#ifndef M_LN2
#    define M_LN2 0.69314718055994530942 /* log_e(2) */
#endif

#if 1 
/* Use float functions */
#define SINF(x)        sinf(x)
#define COSF(x)        cosf(x)
#define FABSF(x)       fabsf(x)
#define FLOORF(x)      floorf(x)
#define EXPF(x)        expf(x)
#define POWF(x,p)      powf(x,p)
#define COPYSIGNF(s,d) copysignf(s,d)
#define LRINTF(x)      lrintf(x)

#else
/* Use double functions */
#define SINF(x)        sin(x)
#define COSF(x)        cos(x)
#define FABSF(x)       fabs(x)
#define FLOORF(x)      floor(x)
#define EXPF(x)        exp(x)
#define POWF(x,p)      pow(x)
#define COPYSIGNF(s,d) copysign(s,d)
#define LRINTF(x)      lrint(x)

#endif

#endif
