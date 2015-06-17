/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_DEBUG_H
#define HAKA_DEBUG_H

#include <haka/config.h>

#ifdef HAKA_DEBUG
	void break_required();
	#define BREAKPOINT     break_required()
#else
	#define BREAKPOINT
#endif

#endif /* HAKA_DEBUG_H */
