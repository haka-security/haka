/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CTL_H
#define CTL_H


bool prepare_ctl_server(const char *ctl_socket_file);
bool start_ctl_server(void);
void stop_ctl_server(void);

#endif /* CTL_H */
