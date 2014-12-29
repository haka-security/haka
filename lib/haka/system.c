/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

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
		LOG_FATAL(core, "%s", errno_error(errno));
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
		error("memory error");
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

const char *_get_config_value(const char *name, const char *def)
{
	const char *env = getenv(name);
	return env ? env : def;
}

const char *haka_path()
{
	return GET_CONFIG_VALUE(HAKA_PATH);
}

void haka_exit()
{
	kill(getpid(), SIGTERM);
}

bool get_memory_size(size_t *vmsize, size_t *rss)
{
	FILE *fp;
	static size_t page_size = 0;

	if (!page_size) {
		page_size = (size_t)sysconf( _SC_PAGESIZE);
	}

	if ((fp = fopen("/proc/self/statm", "r")) == NULL) {
		error("cannot get memory information");
		return false;
	}

	if (fscanf(fp, "%zd%zd", vmsize, rss) != 2) {
		error("cannot get memory information");
		fclose(fp);
		*vmsize = 0;
		*rss = 0;
		return false;
	}

	*vmsize = (*vmsize * page_size) / 1024;
	*rss = (*rss * page_size) / 1024;

	fclose(fp);
	return true;
}

bool mkdir_path(const char *_path, int mode)
{
	struct stat st;
	char *cur;
	bool ret = true;
	char *path = strdup(_path);

	if (!path) {
		error("Memory error");
		return false;
	}

	cur = path;

	while (true)
	{
		char *sep = strchr(cur, '/');
		if (cur != sep) {
			if (sep) *sep = '\0';

			if (stat(path, &st) != 0) {
				if (mkdir(path, mode) != 0 && errno != EEXIST) {
					ret = false;
				}
			}
			else if (!S_ISDIR(st.st_mode)) {
				ret = false;
			}

			if (sep) *sep = '/';

			if (!sep || !ret) break;
		}

		cur = sep+1;
	}

	free(path);
	return ret;
}
