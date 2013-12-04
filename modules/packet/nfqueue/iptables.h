/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _IPTABLES_H
#define _IPTABLES_H

int apply_iptables(const char *conf);
int save_iptables(const char *table, char **conf);

#endif /* _IPTABLES_H */
