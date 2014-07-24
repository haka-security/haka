/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module geoip

%include "haka/lua/swig.si"
%include "haka/lua/object.si"
%include "haka/lua/ipv4-addr.si"

%{
#include "geoip.h"

#include <haka/error.h>

static void country(struct ipv4_addr *addr, char **TEMP_OUTPUT)
{
	*TEMP_OUTPUT = malloc(3);
	if (!*TEMP_OUTPUT) {
		error(L"memory error");
		return;
	}

	if (!geoip_lookup_country(addr->addr, *TEMP_OUTPUT)) {
		free(*TEMP_OUTPUT);
		*TEMP_OUTPUT = NULL;
		return;
	}

	(*TEMP_OUTPUT)[2] = '\0';
}
%}

void country(struct ipv4_addr *addr, char **TEMP_OUTPUT);
