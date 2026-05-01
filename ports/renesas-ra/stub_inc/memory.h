/* Minimal memory.h stub for freestanding ARM */
#ifndef _MEMORY_H
#define _MEMORY_H

#include <stddef.h>

void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
void *memmove(void *, const void *, size_t);

#endif /* _MEMORY_H */
