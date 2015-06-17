/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_COLORS_H
#define HAKA_COLORS_H

#include <haka/types.h>


#define CLEAR   "\e[0m"
#define BOLD    "\e[1m"
#define BLACK   "\e[30m"
#define RED     "\e[31m"
#define GREEN   "\e[32m"
#define YELLOW  "\e[33m"
#define BLUE    "\e[34m"
#define MAGENTA "\e[35m"
#define CYAN    "\e[36m"
#define WHITE   "\e[37m"

bool        colors_supported(int fd);
const char *c(const char *color, bool supported);

#endif /* HAKA_COLORS_H */
