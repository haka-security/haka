
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <haka/timer.h>
#include <haka/error.h>
#include <haka/thread.h>
#include <haka/compiler.h>


struct timer {
	timer_t         id;
	bool            armed;
	timer_callback  callback;
	void           *data;
};

static void timer_handler(int sig, siginfo_t *si, void *uc)
{
	struct timer *timer = (struct timer *)si->si_value.sival_ptr;

	if (timer) {
		timer->callback(timer_getoverrun(timer->id), timer->data);
	}
}

INIT static void _timer_init()
{
	timer_init_thread();
}

bool timer_init_thread()
{
	struct sigaction sa;
	sigset_t mask;

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timer_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		error(L"%s", errno_error(errno));
		return false;
	}

	sigfillset(&mask);
	sigaddset(&mask, SIGALRM);
	if (!thread_sigmask(SIG_UNBLOCK, &mask, NULL)) {
		return false;
	}

	return true;
}

struct timer *timer_init(timer_callback callback, void *user)
{
	struct sigevent sev;

	struct timer *timer = malloc(sizeof(struct timer));
	if (!timer) {
		error(L"memory error");
		return NULL;
	}

	timer->armed = false;
	timer->callback = callback;
	timer->data = user;

	sev.sigev_notify = SIGEV_THREAD_ID;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_ptr = timer;
	sev._sigev_un._tid = syscall(SYS_gettid);

	if (timer_create(CLOCK_MONOTONIC, &sev, &timer->id)) {
		free(timer);
		error(L"timer creation error: %s", errno_error(errno));
		return NULL;
	}

	return timer;
}

void timer_destroy(struct timer *timer)
{
	timer_delete(timer->id);
	free(timer);
}

bool timer_once(struct timer *timer, time_us delay)
{
	struct itimerspec ts;
	memset(&ts, 0, sizeof(ts));

	ts.it_value.tv_sec = delay / 1000000;
	ts.it_value.tv_nsec = (delay % 1000000) * 1000;

	if (timer_settime(timer->id, 0, &ts, NULL)) {
		error(L"%s", errno_error(errno));
		return false;
	}
	return true;
}

bool timer_repeat(struct timer *timer, time_us delay)
{
	struct itimerspec ts;

	ts.it_value.tv_sec = delay / 1000000;
	ts.it_value.tv_nsec = (delay % 1000000) * 1000;
	ts.it_interval = ts.it_value;

	if (timer_settime(timer->id, 0, &ts, NULL)) {
		error(L"%s", errno_error(errno));
		return false;
	}
	return true;
}

bool timer_stop(struct timer *timer)
{
	struct itimerspec ts;
	memset(&ts, 0, sizeof(ts));

	if (timer_settime(timer->id, 0, &ts, NULL)) {
		error(L"%s", errno_error(errno));
		return false;
	}
	return true;
}
