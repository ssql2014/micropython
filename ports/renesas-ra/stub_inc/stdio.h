/* Minimal stdio.h stub for freestanding ARM */
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t n, const char *fmt, ...);
int puts(const char *s);
int vprintf(const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);

typedef struct __FILE {
    int fd;
} FILE;

extern FILE _stdin;
extern FILE _stdout;
extern FILE _stderr;

#define stdin  (&_stdin)
#define stdout (&_stdout)
#define stderr (&_stderr)

#endif /* _STDIO_H */
