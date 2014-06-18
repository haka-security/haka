/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <haka/system.h>
#include <haka/error.h>
#include <haka/log.h>
#include <haka/container/vector.h>


static struct vector fatal_cleanup = VECTOR_INIT(void *, NULL);

static void call_fatal_cleanup(void)
{
	int i;
	const int size = vector_count(&fatal_cleanup);
	for (i=0; i<size; ++i) {
		void (*func)() = vector_getvalue(&fatal_cleanup, void *, i);
		(*func)();
	}
}

/*
 * Signal handler for fatal signals
 */
static void fatal_error_signal(int sig, siginfo_t *si, void *uc)
{
	call_fatal_cleanup();

	/* Execute default handler */
	signal(sig, SIG_DFL);
	raise(sig);
}

INIT static void system_init()
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = fatal_error_signal;

	if (sigaction(SIGSEGV, &sa, NULL) ||
	    sigaction(SIGILL, &sa, NULL) ||
	    sigaction(SIGFPE, &sa, NULL) ||
	    sigaction(SIGABRT, &sa, NULL)) {
		messagef(HAKA_LOG_FATAL, L"core", L"%s", errno_error(errno));
		abort();
	}
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

void fatal_exit(int rc)
{
	call_fatal_cleanup();
	_exit(rc);
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
