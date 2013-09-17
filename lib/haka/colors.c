
#include <unistd.h>

#include <haka/colors.h>


bool colors_supported(int fd)
{
	return isatty(fd);
}

const char *c(const char *color, bool supported)
{
	if (supported) return color;
	else return "";
}
