/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <editline/readline.h>

#include <haka/error.h>

#include <haka/luadebug/user.h>


static bool readline_initialized = false;
static mutex_t active_readline_session_mutex = MUTEX_INIT;
static struct luadebug_user *current = NULL;

static int empty_generator(const char *text, int state)
{
	return 0;
}

static char **complete(const char *text, int start, int end)
{
	generator_callback *generator = current->completion(rl_line_buffer, start);
	if (generator) {
		return rl_completion_matches(text, generator);
	}
	else {
		return NULL;
	}
}

static bool start(struct luadebug_user *data, const char *name)
{
	mutex_lock(&active_readline_session_mutex);

	if (!readline_initialized) {
		rl_initialize();
		readline_initialized = true;
	}

	current = data;

	rl_basic_word_break_characters = " \t\n`@$><=;|&{(";
	rl_readline_name = (char *)name;
	rl_attempted_completion_function = complete;
	rl_completion_entry_function = empty_generator;
	using_history();

	return true;
}

static char *my_readline(struct luadebug_user *data, const char *prompt)
{
	return readline(prompt);
}

static void addhistory(struct luadebug_user *data, const char *line)
{
	add_history(line);
}

static bool stop(struct luadebug_user *data)
{
	mutex_unlock(&active_readline_session_mutex);
	current = NULL;
	return true;
}

static void print(struct luadebug_user *data, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

static bool check(struct luadebug_user *data)
{
	return true;
}

static void destroy(struct luadebug_user *data)
{
	free(data);
}

struct luadebug_user *luadebug_user_readline()
{
	struct luadebug_user *ret = malloc(sizeof(struct luadebug_user));
	if (!ret) {
		error("memory error");
		return NULL;
	}

	luadebug_user_init(ret);
	ret->start = start;
	ret->readline = my_readline;
	ret->addhistory = addhistory;
	ret->stop = stop;
	ret->print = print;
	ret->check = check;
	ret->destroy = destroy;

	return ret;
}
