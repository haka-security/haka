
#ifndef _LUADEBUG_USER_H
#define _LUADEBUG_USER_H

#include <haka/types.h>

typedef char *generator_callback(const char *text, int state);

struct luadebug_user {
	generator_callback *(*completion)(const char *line, int start);

	/* User functions */
	void      (*start)(struct luadebug_user *data, const char *name);
	char     *(*readline)(struct luadebug_user *data, const char *prompt);
	void      (*addhistory)(struct luadebug_user *data, const char *line);
	void      (*stop)(struct luadebug_user *data);
	void      (*print)(struct luadebug_user *data, const char *format, ...);
	void      (*destroy)(struct luadebug_user *data);
};

struct luadebug_user *luadebug_user_readline();

#endif /* _LUADEBUG_USER_H */
