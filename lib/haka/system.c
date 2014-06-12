/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <haka/system.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/container/vector.h>


static struct vector fatal_cleanup = VECTOR_INIT(void *, NULL);

/*
 * Signal handler for fatal signals
 */
static void fatal_error_signal(int sig)
{
	int i;
	const int size = vector_count(&fatal_cleanup);
	for (i=0; i<size; ++i) {
		void (*func)() = vector_getvalue(&fatal_cleanup, void *, i);
		(*func)();
	}

	/* Execute default handler */
	signal(sig, SIG_DFL);
	raise(sig);
}

INIT static void system_init()
{
	signal(SIGSEGV, fatal_error_signal);
	signal(SIGILL, fatal_error_signal);
	signal(SIGFPE, fatal_error_signal);
	signal(SIGABRT, fatal_error_signal);
}

FINI static void system_final()
{
	vector_destroy(&fatal_cleanup);
}

bool system_register_fatal_cleanup(void (*callback)())
{
	void (**func)() = (void (**)()) vector_push(&fatal_cleanup, void *);
	if (!func) {
		error(L"memory error");
		return false;
	}

	*func = callback;

	return true;
}

const char *haka_path()
{
	const char *haka_path = getenv("HAKA_PATH");
	return haka_path ? haka_path : HAKA_PREFIX;
}

void haka_exit()
{
	messagef(HAKA_LOG_INFO, L"core", L"request to stop haka received");
	kill(getpid(), SIGTERM);
}
