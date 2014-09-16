/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module geoip

%include "haka/lua/swig.si"
%include "haka/lua/object.si"
%include "haka/lua/ipv4-addr.si"

%{
#include "geoip.h"
%}

%nodefaultctor;
%nodefaultdtor;

struct geoip_handle {
	%extend{
		~geoip_handle() {
			geoip_destroy($self);
		}

		const char *country(struct ipv4_addr *addr) {
			static char country_code[3];

			if (!geoip_lookup_country($self, addr->addr, country_code)) {
				return NULL;
			}

			return country_code;
		}
	}
};

%rename(open) geoip_initialize;
%newobject geoip_initialize;
struct geoip_handle *geoip_initialize(const char *database);
