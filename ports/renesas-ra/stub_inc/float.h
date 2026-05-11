/* Minimal float.h stub for freestanding ARM - use compiler builtins */
#ifndef _FLOAT_H
#define _FLOAT_H

/* Let the compiler provide these via builtins */
#define NAN (__builtin_nanf(""))
#define INFINITY (__builtin_inff())

#endif /* _FLOAT_H */
