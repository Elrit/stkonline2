#include "errio.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int eprintf(const char *const fmt, ...) {
	va_list args;
	va_start(args, fmt);
	const int res = vfprintf(stderr, fmt, args);
	va_end(args);
	return res;
}

void perrorf(const char *const fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	eprintf(": %s\n", strerror(errno));
}
