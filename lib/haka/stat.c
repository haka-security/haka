
#include <stdio.h>
#include <stdarg.h>

#include <haka/stat.h>
#include <haka/types.h>


size_t stat_printf(FILE *out, const char *format, ...)
{
	size_t ret;
	va_list args;
	va_start(args, format);
	ret = vfprintf(out, format, args);
	va_end(args);
	return ret;
}

#define UNITS_COUNT  4

size_t stat_format_bytes(FILE *out, size_t v)
{
	static const char *units[UNITS_COUNT] = {
		"k", "M", "G", "T"
	};

	if (v < 1000) {
		return stat_printf(out, "%d", v);
	}
	else {
		int i = 0, digit = 0, rem;
		while (v >= 1000000 && i <= UNITS_COUNT) {
			v /= 1000;
			++i;
		}

		rem = v;
		while (rem > 1000) {
			rem /= 10;
			++digit;
		}

		if (i < UNITS_COUNT) return stat_printf(out, "%.*f%s", 3-digit, v / 1000.0f, units[i]);
		else return stat_printf(out, "%d%s", v, units[UNITS_COUNT-1]);
	}
}
