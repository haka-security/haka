
#ifndef _LUADEBUG_USER_H
#define _LUADEBUG_USER_H

#include <haka/types.h>
#include <haka/thread.h>

typedef char *generator_callback(const char *text, int state);

struct luadebug_user {
	atomic_t   refcount;
	generator_callback *(*completion)(const char *line, int start);

	/* User functions */
	bool      (*start)(struct luadebug_user *data, const char *name);
	char     *(*readline)(struct luadebug_user *data, const char *prompt);
	void      (*addhistory)(struct luadebug_user *data, const char *line);
	bool      (*stop)(struct luadebug_user *data);
	void      (*print)(struct luadebug_user *data, const char *format, ...);
	bool      (*check)(struct luadebug_user *data);
	void      (*destroy)(struct luadebug_user *data);
};

void luadebug_user_init(struct luadebug_user *user);
void luadebug_user_addref(struct luadebug_user *user);
void luadebug_user_release(struct luadebug_user **user);

struct luadebug_user *luadebug_user_readline();

struct luadebug_user *luadebug_user_remote(int fd);
void                  luadebug_user_remote_server(int fd, struct luadebug_user *user);

#endif /* _LUADEBUG_USER_H */
