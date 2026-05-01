/* Minimal unistd.h stub for freestanding ARM */
#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

void _exit(int);
int close(int);
int read(int, void *, size_t);
int write(int, const void *, size_t);

#endif /* _UNISTD_H */
