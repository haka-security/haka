/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
