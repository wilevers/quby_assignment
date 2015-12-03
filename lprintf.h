#include <stdarg.h>

#ifndef LPRINTF_H
#define LPRINTF_H

typedef enum {
	fatal,
	error,
	warning,
	info
} loglevel;

void set_loglevel(int level);
void lprintf(loglevel level, const char *fmt, ...);
void vlprintf(loglevel level, const char *fmt, va_list args);

#endif
