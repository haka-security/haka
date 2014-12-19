/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <stdio.h>

#include <haka/utils.h>

size_t safe_string(char dst[], const char src[], size_t size)
{
	size_t i, j;

	for (i = 0, j = 0; i < size; i++) {
		if (src[i] >= 0x20 && src[i] <= 0x7e) {
			dst[j] = src[i];
			j++;
		} else {
			j += snprintf(&dst[j], 5, "\\x%.2hhx", src[i]);
		}
	}

	dst[j] = '\0';
	return j;
}
