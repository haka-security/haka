
#include <stdio.h>
#include <stdarg.h>

#include <haka/stat.h>
#include <haka/types.h>

bool stat_printf(FILE *out, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(out, format, args);
	va_end(args);
	return true;
}

size_t format_bytes(size_t v, char *c)
{
	if (v > (1 << 30)) {
		*c = 'g';
		return (v / (1 << 30));
	}
	else if (v > (1 << 20)) {
		*c = 'm';
		return (v / (1 << 20));
	}
	else if (v > (1 << 10)) {
		*c = 'k';
		return (v / (1 << 10));
	}
	else {
		*c = ' ';
		return v;
	}
}
