
#ifndef _HAKA_TIMER_H
#define _HAKA_TIMER_H

#include <haka/types.h>
#include <haka/time.h>


struct timer;

typedef void (*timer_callback)(int count, void *data);

bool timer_init_thread();
struct timer *timer_init(timer_callback callback, void *user);
void timer_destroy(struct timer *timer);
bool timer_once(struct timer *timer, struct time *delay);
bool timer_repeat(struct timer *timer, struct time *delay);
bool timer_stop(struct timer *timer);

#endif /* _HAKA_TIMER_H */
