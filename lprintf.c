#include <stdio.h>

#include "lprintf.h"

#ifdef NDEBUG
enum { default_level = error };
#else
enum { default_level = warning };
#endif

static int current_level = default_level;

void set_loglevel(int level)
{
	if (level >= 0) {
		current_level = level;
	}
}

void lprintf(loglevel level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlprintf(level, fmt, args);
	va_end(args);
}

void vlprintf(loglevel level, const char *fmt, va_list args)
{
	if (current_level >= level) {
		vfprintf(stderr, fmt, args);
	}
}
