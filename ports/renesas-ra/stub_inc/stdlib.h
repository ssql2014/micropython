/* Minimal stdlib.h stub for freestanding ARM */
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);
void abort(void);
void exit(int);
int atoi(const char *);

#endif /* _STDLIB_H */

/* Additional math functions */
int abs(int) __attribute__((const));
long labs(long) __attribute__((const));
