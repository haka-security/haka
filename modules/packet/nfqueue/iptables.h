/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _IPTABLES_H
#define _IPTABLES_H

#include <haka/types.h>

#define HAKA_TARGET         "HAKA"
#define HAKA_TARGET_PRE     HAKA_TARGET "-PRE"
#define HAKA_TARGET_OUT     HAKA_TARGET "-OUT"

int apply_iptables(const char *table, const char *conf, bool noflush);
int save_iptables(const char *table, char **conf, bool all_targets);

#endif /* _IPTABLES_H */
