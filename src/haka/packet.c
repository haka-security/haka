/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>

#include <haka/config.h>
#include <haka/luabinding.h>
#include <haka/packet.h>
#include <haka/types.h>

#include "packet.h"
#include "thread.h"

#ifdef HAKA_FFI

void packet_receive_wrapper_wrap(void *_state, struct receive_result *res)
{
	struct thread_state *state;
	state = (struct thread_state *)_state;

	if (state == NULL) return;

	packet_receive_wrapper(state, &res->pkt, &res->has_interrupts, &res->stop);
}

#else

int packet_receive_wrapper_wrap()
{
	struct packet *pkt;
	bool has_interrupts;
	bool stop;

	// push result on stack
	packet_receive_wrapper(state, &pkt, &has_interrupts, &stop);
}

#endif
