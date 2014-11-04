/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/compiler.h>
#include <haka/luabinding.h>

LUA_BIND_INIT(main_loop);

#ifdef HAKA_FFI

struct receive_result {
	struct packet *pkt;
	bool has_interrupts;
	bool stop;
};

void packet_receive_wrapper_wrap(void *_state, struct receive_result *res);

#else

#endif
