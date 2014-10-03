/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/thread.h>
#include <haka/log.h>
#include <haka/lua/state.h>
#include <haka/lua/luautils.h>

#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#if HAKA_LUAJIT
#include <luajit.h>
#endif

#include <haka/luadebug/debugger.h>
#include <haka/luadebug/user.h>
#include "complete.h"
#include "debugger.h"
#include "utils.h"

#define MODULE    SECTION_LUA


struct luadebug_debugger {
	lua_State                   *top_L;
	lua_State                   *L;
	struct luadebug_complete     complete;

	bool                         active;
	bool                         break_immediatly;
	unsigned int                 break_depth;
	int                          stack_depth;
	int                          list_line;
	lua_Debug                    frame;
	int                          frame_index;
	int                          breakpoints;
	int                          env_index;
	int                          last_line;
	const char                  *last_source;
	char                        *last_command;
	struct luadebug_user        *user;
};

static mutex_t active_session_mutex = MUTEX_INIT;
static struct luadebug_debugger *current_session;
static mutex_t current_user_mutex;
static struct luadebug_user *current_user;
static atomic_t break_required = 0;
static atomic_t running_debugger = 0;

#define LIST_DEFAULT_LINE   10


static void dump_frame(lua_State *L, lua_Debug *ar, struct luadebug_user *user)
{
	lua_getinfo(L, "Snl", ar);

	if (!strcmp(ar->what, "C")) {
		user->print(user, "[C]" CLEAR ": in function '" MAGENTA "%s" CLEAR "'\n",
				ar->name);
	}
	else if (!strcmp(ar->what, "main")) {
		user->print(user, "%s:%d" CLEAR ": in the main chunk\n",
				ar->short_src, ar->currentline);
	}
	else if (!strcmp(ar->what, "Lua")) {
		user->print(user, "%s:%d" CLEAR ": in function '" MAGENTA "%s" CLEAR "'\n",
				ar->short_src, ar->currentline, ar->name);
	}
	else if (!strcmp(ar->what, "tail")) {
		user->print(user, CLEAR "in tail call\n");
	}
	else {
		user->print(user, "%s\n" CLEAR, ar->what);
	}
}

static bool dump_backtrace(struct luadebug_debugger *session, const char *option)
{
	int i;
	lua_Debug ar;

	session->user->print(session->user, "Backtrace\n");

	for (i = 0; lua_getstack(session->L, i, &ar); ++i) {
		if (i == session->frame_index) {
			session->user->print(session->user, RED BOLD " =>" CLEAR "%d\t" CYAN, i);
		}
		else {
			session->user->print(session->user, "  #%d\t" CYAN, i);
		}

		dump_frame(session->L, &ar, session->user);
	}

	return false;
}

static int get_stack_depth(lua_State *L)
{
	int i;
	lua_Debug ar;

	for (i = 0; lua_getstack(L, i, &ar); ++i);
	return i;
}

static bool change_frame(struct luadebug_debugger *session, const char *option)
{
	const int index = option ? atoi(option) : session->frame_index;

	if (!lua_getstack(session->L, index, &session->frame)) {
		session->user->print(session->user, RED "invalid frame index '%d'\n" CLEAR, index);
	}
	else {
		UNUSED int env_index;

		session->user->print(session->user, "  #%d\t" CYAN, index);
		dump_frame(session->L, &session->frame, session->user);

		session->frame_index = index;

		env_index = capture_env(session->L, index);
		assert(env_index != -1);

		lua_replace(session->L, session->env_index);

		session->list_line = session->frame.currentline - LIST_DEFAULT_LINE/2;
	}

	return false;
}

static bool dump_stack(struct luadebug_debugger *session, const char *option)
{
	int i, h;
	const int index = option ? atoi(option) : -1;

	h = lua_gettop(session->L);

	if (index < 0) {
		session->user->print(session->user, "Stack (size=%d)\n", h-1);
		for (i = 1; i < h; ++i) {
			session->user->print(session->user, "  #%d\t", i);
			pprint(session->L, session->user, i, false, NULL);
		}
	}
	else if (index > 0 && index < h) {
		session->user->print(session->user, "  #%d\t", index);
		pprint(session->L, session->user, index, true, NULL);
	}
	else {
		session->user->print(session->user, RED "invalid stack index '%d'\n" CLEAR, index);
	}

	return false;
}

static bool dump_local(struct luadebug_debugger *session, const char *option)
{
	int i;
	const char *local;
	const int index = option ? atoi(option) : -1;

	if (index < 0) {
		session->user->print(session->user, "Locals\n");
		for (i=1; (local = lua_getlocal(session->L, &session->frame, i)); ++i) {
			session->user->print(session->user, "  #%d\t%s = ", i, local);
			pprint(session->L, session->user, -1, false, NULL);
			lua_pop(session->L, 1);
		}
	}
	else {
		local = lua_getlocal(session->L, &session->frame, index);
		if (!local) {
			session->user->print(session->user, RED "invalid local index '%d'\n" CLEAR, index);
		}
		else {
			session->user->print(session->user, "  #%d\t%s = ", index, local);
			pprint(session->L, session->user, -1, true, NULL);
			lua_pop(session->L, 1);
		}
	}

	return false;
}

static bool dump_upvalue(struct luadebug_debugger *session, const char *option)
{
	int i;
	const char *local;
	const int index = option ? atoi(option) : -1;

	lua_getinfo(session->L, "f", &session->frame);

	if (index < 0) {
		session->user->print(session->user, "Up-values\n");
		for (i=1; (local = lua_getupvalue(session->L, -1, i)); ++i) {
			session->user->print(session->user, "  #%d\t%s = ", i, local);
			pprint(session->L, session->user, -1, false, NULL);
			lua_pop(session->L, 1);
		}
	}
	else {
		local = lua_getupvalue(session->L, -1, index);
		if (!local) {
			session->user->print(session->user, RED "invalid up-value index '%d'\n" CLEAR, index);
		}
		else {
			session->user->print(session->user, "  #%d\t%s = ", index, local);
			pprint(session->L, session->user, -1, true, NULL);
			lua_pop(session->L, 1);
		}
	}

	lua_pop(session->L, 1);
	return false;
}

static bool push_breakpoints(struct luadebug_debugger *session, lua_Debug *ar)
{
	lua_getinfo(session->L, "nS", ar);

	if (!strcmp(ar->what, "main") || !strcmp(ar->what, "Lua")) {
		lua_rawgeti(session->L, LUA_REGISTRYINDEX, session->breakpoints);
		assert(!lua_isnil(session->L, -1));
		lua_getfield(session->L, -1, ar->source);
		if (lua_isnil(session->L, -1)) {
			lua_pop(session->L, 1);
			lua_newtable(session->L);
			lua_pushvalue(session->L, -1);
			lua_setfield(session->L, -3, ar->source);
		}
		lua_remove(session->L, -2);
		return true;
	}
	else {
		return false;
	}
}

static bool has_breakpoint(struct luadebug_debugger *session, lua_Debug *ar)
{
	bool breakpoint = false;

	if (push_breakpoints(session, ar)) {
		lua_pushnumber(session->L, ar->currentline);
		lua_gettable(session->L, -2);
		breakpoint = !lua_isnil(session->L, -1);
		lua_pop(session->L, 2);
	}

	return breakpoint;
}

static bool add_breakpoint(struct luadebug_debugger *session, const char *option)
{
	const int line = option ? atoi(option) : session->frame.currentline;

	if (push_breakpoints(session, &session->frame)) {
		lua_pushnumber(session->L, line);
		lua_pushboolean(session->L, true);
		lua_settable(session->L, -3);
		lua_pop(session->L, 1);

		session->user->print(session->user, "breakpoint added at '%s:%d'\n", session->frame.short_src, line);
	}
	else {
		session->user->print(session->user, RED "invalid breakpoint location\n" CLEAR);
	}
	return false;
}

static bool remove_breakpoint(struct luadebug_debugger *session, const char *option)
{
	const int line = option ? atoi(option) : session->frame.currentline;

	if (push_breakpoints(session, &session->frame)) {
		lua_pushnumber(session->L, line);
		lua_gettable(session->L, -2);
		if (lua_isnil(session->L, -1)) {
			lua_pop(session->L, 1);
			session->user->print(session->user, RED "not breakpoint at '%s:%d'\n" CLEAR, session->frame.short_src, line);
		}
		else {
			lua_pop(session->L, 1);
			lua_pushnumber(session->L, line);
			lua_pushnil(session->L);
			lua_settable(session->L, -3);
			session->user->print(session->user, "breakpoint removed from '%s:%d'\n", session->frame.short_src, line);
		}

		lua_pop(session->L, 1);
	}
	else {
		session->user->print(session->user, RED "invalid breakpoint location\n" CLEAR);
	}
	return false;
}

static bool print_line_char(lua_State *L, int c, int line, int current, int *count,
		unsigned int start, unsigned int end, bool show_breakpoints, struct luadebug_user *user)
{
	bool breakpoint = false;

	if (show_breakpoints) {
		lua_pushnumber(L, line);
		lua_gettable(L, -2);
		breakpoint = !lua_isnil(L, -1);
		lua_pop(L, 1);
	}

	if ((*count)++ == 0) {
		if (line >= start && line <= end) {
			if (line == current) {
				if (breakpoint) {
					user->print(user, RED "%4d" BOLD "#> " CLEAR, line);
				}
				else {
					user->print(user, RED "%4d" BOLD "=> " CLEAR, line);
				}
			}
			else {
				if (breakpoint) {
					user->print(user, YELLOW "%4d" BOLD "#  " CLEAR, line);
				}
				else {
					user->print(user, YELLOW "%4d:  " CLEAR, line);
				}
			}
		}
	}

	if (line >= start && line <= end) {
		if (c == '\t') user->print(user, "    ");
		else user->print(user, "%c", c);
	}
	else if (line > end) {
		return true;
	}

	if (c == '\n') {
		return true;
	}

	return false;
}

static bool print_file_line(lua_State *L, FILE *file, int num, int current,
		unsigned int start, unsigned int end, bool show_breakpoints, struct luadebug_user *user)
{
	int c;
	int count = 0;

	while ((c = fgetc(file)) > 0) {
		if (print_line_char(L, c, num, current, &count, start, end, show_breakpoints, user)) {
			return true;
		}
	}

	if (count) {
		user->print(user, "\n");
	}

	return false;
}

static bool print_line(lua_State *L, const char **string, int num, int current,
		unsigned int start, unsigned int end, bool show_breakpoints, struct luadebug_user *user)
{
	int c;
	int count = 0;

	while ((c = *((*string)++)) > 0) {
		if (print_line_char(L, c, num, current, &count, start, end, show_breakpoints, user)) {
			return true;
		}
	}

	if (count) {
		user->print(user, "\n");
	}

	return false;
}

static void dump_source(struct luadebug_debugger *session, lua_Debug *ar, unsigned int start, unsigned int end)
{
	const bool breakpoints = push_breakpoints(session, ar);

	lua_getinfo(session->L, "S", ar);

	if (ar->source[0] == '@') {
		int line = 1;
		FILE *file = fopen(ar->source+1, "r");
		if (file) {
			while (print_file_line(session->L, file, line++, ar->currentline, start, end, breakpoints, session->user));

			fclose(file);
		}
		else {
			session->user->print(session->user, RED "cannot open '%s'\n" CLEAR, ar->source+1);
		}
	}
	else if (ar->source[0] == '=') {
		session->user->print(session->user, "%s\n", ar->source+1);
	}
	else {
		int line = 1;
		const char *source = ar->source;

		while (print_line(session->L, &source, line++, ar->currentline, start, end, breakpoints, session->user));
	}

	if (breakpoints) {
		lua_pop(session->L, 1);
	}
}

static bool check_token(const char *line, const char *token, const char *short_token, const char **option)
{
	size_t len;

	*option = NULL;

	if (strcmp(line, token) == 0) {
		return true;
	}

	len = strlen(token);
	if (strncmp(line, token, len) == 0) {
		if (line[len] == ' ') {
			if (line[len+1] != 0) {
				*option = line + len + 1;
			}
			return true;
		}
	}

	if (short_token) {
		if (strcmp(line, short_token) == 0) {
			return true;
		}

		len = strlen(short_token);
		if (strncmp(line, short_token, len) == 0) {
			if (line[len] == ' ') {
				if (line[len+1] != 0) {
					*option = line + len + 1;
				}
				return true;
			}
		}
	}

	return false;
}

static bool print_exp(struct luadebug_debugger *session, const char *option)
{
	if (!option) {
		return false;
	}

	int status;
	char *print_line = malloc(7 + strlen(option) + 1);

	strcpy(print_line, "return ");
	strcpy(print_line+7, option);

	status = luaL_loadbuffer(session->L, print_line, strlen(print_line), "stdin");
	if (status) {
		session->user->print(session->user, RED "%s\n" CLEAR, lua_tostring(session->L, -1));
		lua_pop(session->L, 1);
	}
	else {
		lua_pushvalue(session->L, session->env_index);

#if HAKA_LUA52
		lua_setupvalue(session->L, -2, 1);
#else
		lua_setfenv(session->L, -2);
#endif

		execute_print(session->L, session->user, true, NULL);
		lua_pop(session->L, 1);
	}

	free(print_line);
	return false;
}

static bool list_source(struct luadebug_debugger *session, const char *option)
{
	if (option) {
		if (strcmp(option, "-") == 0) {
			session->list_line -= 2*LIST_DEFAULT_LINE + 1;
		}
		else if (strcmp(option, "+") == 0) {
		}
		else {
			const int line = atoi(option);
			session->list_line = line - LIST_DEFAULT_LINE/2;
		}
	}

	if (session->list_line < 0) {
		session->list_line = 0;
	}

	dump_source(session, &session->frame, session->list_line, session->list_line + LIST_DEFAULT_LINE);
	session->list_line += LIST_DEFAULT_LINE+1;
	return false;
}

static bool do_step(struct luadebug_debugger *session, const char *option)
{
	session->break_immediatly = true;
	return true;
}

static bool do_next(struct luadebug_debugger *session, const char *option)
{
	session->break_immediatly = true;
	session->break_depth = session->stack_depth;
	return true;
}

static bool do_finish(struct luadebug_debugger *session, const char *option)
{
	if (session->stack_depth > 0) {
		session->break_immediatly = true;
		session->break_depth = session->stack_depth-1;
	}
	return true;
}

static bool do_continue(struct luadebug_debugger *session, const char *option)
{
	return true;
}

static bool do_help(struct luadebug_debugger *session, const char *option);

typedef bool (*command_callback)(struct luadebug_debugger *, const char *);

struct command {
	const char      *keyword;
	const char      *alt_keyword;
	const char      *description;
	bool             option;
	command_callback callback;
};

static struct command commands[] = {
	{
		keyword:     "bt",
		description: BOLD "bt" CLEAR "            dump the backtrace",
		callback:    dump_backtrace
	},
	{
		keyword:     "frame",
		alt_keyword: "f",
		description: BOLD "frame N" CLEAR "       change the current frame",
		option:      true,
		callback:    change_frame
	},
	{
		keyword:     "stack",
		description: BOLD "stack" CLEAR "         dump the full Lua stack\n"
		             BOLD "stack N" CLEAR "       dump the element at index N in the Lua stack",
		option:      true,
		callback:    dump_stack
	},
	{
		keyword:     "local",
		description: BOLD "local" CLEAR "         dump all Lua locals\n"
		             BOLD "local N" CLEAR "       dump the Nth local",
		option:      true,
		callback:    dump_local
	},
	{
		keyword:     "upvalue",
		description: BOLD "upvalue" CLEAR "       dump all Lua up-values\n"
		             BOLD "upvalue N" CLEAR "     dump the Nth up-value",
		option:      true,
		callback:    dump_upvalue
	},
	{
		keyword:     "print",
		alt_keyword: "p",
		description: BOLD "print exp" CLEAR "     print the expression",
		option:      true,
		callback:    print_exp
	},
	{
		keyword:     "list",
		alt_keyword: "l",
		description: BOLD "list" CLEAR "          show sources\n"
		             BOLD "list [+|-]" CLEAR "    show next or previous lines\n"
		             BOLD "list N" CLEAR "        show sources around line N",
		option:      true,
		callback:    list_source
	},
	{
		keyword:     "break",
		alt_keyword: "b",
		description: BOLD "break" CLEAR "         add breakpoint on current line\n"
		             BOLD "break N" CLEAR "       add breakpoint on line N",
		option:      true,
		callback:    add_breakpoint
	},
	{
		keyword:     "removebreak",
		alt_keyword: "rb",
		description: BOLD "removebreak" CLEAR "   remove breakpoint on current line\n"
		             BOLD "removebreak N" CLEAR " remove breakpoint on line N",
		option:      true,
		callback:    remove_breakpoint
	},
	{
		keyword:     "next",
		alt_keyword: "n",
		description: BOLD "next" CLEAR "          step in the program (through subcalls)",
		callback:    do_next
	},
	{
		keyword:     "step",
		alt_keyword: "s",
		description: BOLD "step" CLEAR "          step in the program",
		callback:    do_step
	},
	{
		keyword:     "finish",
		description: BOLD "finish" CLEAR "        step out of the current function",
		callback:    do_finish
	},
	{
		keyword:     "continue",
		alt_keyword: "c",
		description: BOLD "continue" CLEAR "      continue execution",
		callback:    do_continue
	},
	{
		keyword:     "help",
		description: BOLD "help" CLEAR "          command help",
		callback:    do_help
	},
	{
		keyword:     NULL,
	}
};

static bool do_help(struct luadebug_debugger *session, const char *option)
{
	struct command *iter = commands;

	while (iter->keyword) {
		session->user->print(session->user, "%s\n", iter->description);
		++iter;
	}

	return false;
}

static bool process_command(struct luadebug_debugger *session, const char *line,
		bool with_history)
{
	bool valid_line = false;
	bool cont = false;
	const char *option;
	struct command *iter = commands;

	while (iter->keyword) {
		if (check_token(line, iter->keyword, iter->alt_keyword, &option)) {
			valid_line = true;

			if (iter->option || !option) {
				LUA_STACK_MARK(session->L);
				cont = (*iter->callback)(session, option);
				LUA_STACK_CHECK(session->L, 0);
				break;
			}
		}

		++iter;
	}

	if (!valid_line && strlen(line) == 0) {
		if (session->last_command) {
			cont = process_command(session, session->last_command, false);
		}

		valid_line = false;
	}
	else if (!valid_line)
	{
		session->user->print(session->user, RED "invalid command '%s'\n" CLEAR, line);
	}

	if (valid_line && with_history) {
		session->user->addhistory(session->user, line);

		if (session->last_command) free(session->last_command);
		session->last_command = strdup(line);
	}

	return cont;
}

static char *complete_callback_debug_keyword(struct lua_State *L, struct luadebug_complete *context,
		const char *text, int state)
{
	int text_len;

	if (state == 0) {
		context->index = -1;
	}

	text_len = strlen(text);

	for (++context->index; commands[context->index>>1].keyword; ++context->index) {
		struct command *cmd = &commands[context->index>>1];
		const char *keyword = (context->index & 1) ? cmd->keyword : cmd->alt_keyword;

		if (keyword) {
			const int len = strlen(keyword);
			if (len >= text_len && !strncmp(keyword, text, text_len)) {
				if (cmd->option) {
					return complete_addchar(keyword, ' ');
				}
				else {
					return strdup(keyword);
				}
			}
		}
	}

	return NULL;
}

static const complete_callback completions[] = {
	&complete_callback_debug_keyword,
	NULL
};

static char *command_generator(const char *text, int state)
{
	return complete_generator(current_session->L, &current_session->complete,
			completions, text, state);
}

static const complete_callback lua_completions[] = {
	&complete_callback_lua_keyword,
	&complete_callback_global,
	&complete_callback_fenv,
	&complete_callback_table,
	&complete_callback_swig_get,
	&complete_callback_swig_fn,
	NULL
};

static char *lua_generator(const char *text, int state)
{
	return complete_generator(current_session->L, &current_session->complete,
			lua_completions, text, state);
}

static generator_callback *completion(const char *line, int start)
{
	if (start == 0) {
		return command_generator;
	}
	else {
		if (strncmp(line, "p ", 2) == 0 ||
			strncmp(line, "print ", 6) == 0) {
			return lua_generator;
		}
		else {
			return NULL;
		}
	}
}

static void on_user_error(struct luadebug_debugger *session)
{
	luadebug_debugger_user(NULL);
	luadebug_debugger_stop(session->L);
}

static bool prepare_debugger(struct luadebug_debugger *session)
{
	mutex_lock(&current_user_mutex);
	session->user = current_user;

	if (!session->user) {
		mutex_unlock(&current_user_mutex);
		LOG_ERROR(MODULE, "no input/output handler");
		on_user_error(session);
		return false;
	}

	luadebug_user_addref(session->user);

	mutex_unlock(&current_user_mutex);
	return true;
}

struct process_debugger_data {
	struct luadebug_debugger *session;
	lua_Debug                *ar;
	const char               *reason;
	bool                      show_backtrace;
};

static void process_debugger(void *_data)
{
	struct process_debugger_data *data = _data;
	char *line;
	LUA_STACK_MARK(data->session->L);

	data->session->user->completion = completion;

	if (!data->session->user->start(data->session->user, "debug")) {
		on_user_error(data->session);
		return;
	}

	data->session->list_line = data->ar->currentline - LIST_DEFAULT_LINE/2;
	data->session->frame = *data->ar;
	data->session->frame_index = 0;
	data->session->env_index = capture_env(data->session->L, 0);
	assert(data->session->env_index != -1);
	data->session->complete.stack_env = data->session->env_index;
	data->session->break_immediatly = false;
	data->session->break_depth = -1;

	if (data->reason) {
		data->session->user->print(data->session->user, GREEN "entering debugger" CLEAR ": %s\n",
				data->reason);
	}

	data->session->user->print(data->session->user, "thread: %d\n", thread_getid());

	if (data->show_backtrace) {
		dump_backtrace(data->session, NULL);
	}

	dump_source(data->session, data->ar, data->ar->currentline, data->ar->currentline);

	while ((line = data->session->user->readline(data->session->user, "debug>  "))) {
		const bool cont = process_command(data->session, line, true);
		free(line);

		if (cont) {
			break;
		}
	}

	if (!line) {
		data->session->user->print(data->session->user, "\n");
		data->session->user->print(data->session->user, GREEN "continue" CLEAR "\n");
	}

	lua_pop(data->session->L, 1);

	LUA_STACK_CHECK(data->session->L, 0);

	if (!data->session->user->stop(data->session->user)) {
		on_user_error(data->session);
	}
}

static void process_debugger_cleanup(void *_)
{
	luadebug_user_release(&current_session->user);
	current_session = NULL;
	mutex_unlock(&active_session_mutex);
}

static void enter_debugger(struct luadebug_debugger *session, lua_Debug *ar, const char *reason,
		bool show_backtrace)
{
	struct process_debugger_data data;

	if (!prepare_debugger(session)) {
		return;
	}

	mutex_lock(&active_session_mutex);
	current_session = session;

	data.session = session;
	data.ar = ar;
	data.reason = reason;
	data.show_backtrace = show_backtrace;

	thread_protect(process_debugger, &data, process_debugger_cleanup, NULL);
}

static struct luadebug_debugger *luadebug_debugger_get(struct lua_State *L)
{
	struct luadebug_debugger *ret = NULL;

	lua_getfield(L, LUA_REGISTRYINDEX, "__debugger");
	if (!lua_isnil(L, -1)) {
		ret = lua_getpdebugger(L, -1);
	}

	lua_pop(L, 1);
	return ret;
}

static void lua_debug_hook(lua_State *L, lua_Debug *ar)
{
	struct luadebug_debugger *session;

	LUA_STACK_MARK(L);

	session = luadebug_debugger_get(L);
	assert(session);

	session->L = L;

	switch (ar->event) {
	case LUA_HOOKLINE:
		{
			lua_getinfo(session->L, "S", ar);
			session->stack_depth = get_stack_depth(L);

			if (atomic_get(&break_required)) {
				enter_debugger(session, ar, "general break requested", false);
				atomic_set(&break_required, 0);
			}
			else if (session->break_immediatly && session->stack_depth <= session->break_depth) {
				enter_debugger(session, ar, NULL, false);
			}
			else if ((session->last_source != ar->source || session->last_line != ar->currentline) &&
					has_breakpoint(session, ar)) {
				enter_debugger(session, ar, "breakpoint", false);
			}

			session->last_source = ar->source;
			session->last_line = ar->currentline;
		}
		break;

	case LUA_HOOKCALL:
		break;

	case LUA_HOOKRET:
#if !HAKA_LUA52
	case LUA_HOOKTAILRET:
#endif
		break;

	default:
		break;
	}

	LUA_STACK_CHECK(L, 0);
}

static void luadebug_debugger_activate(struct luadebug_debugger *session)
{
	if (!session->active) {
		struct lua_state *state;

#if HAKA_LUAJIT
		luaJIT_setmode(session->top_L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_OFF);
#endif

		atomic_inc(&running_debugger);

		state = lua_state_get(session->top_L);
		assert(state);

		lua_state_setdebugger_hook(state, &lua_debug_hook);

		session->active = true;

		LOG_INFO(MODULE, "lua debugger activated");
	}
}

struct luadebug_debugger *luadebug_debugger_create(struct lua_State *L, bool break_immediatly)
{
	struct luadebug_debugger *ret;

	lua_getfield(L, LUA_REGISTRYINDEX, "__debugger");
	if (!lua_isnil(L, -1)) {
		error("debugger already attached");
		lua_pop(L, 1);
		return NULL;
	}
	lua_pop(L, 1);

	ret = malloc(sizeof(struct luadebug_debugger));
	if (!ret) {
		error("memory error");
		return NULL;
	}

	ret->top_L = L;
	ret->L = L;
	ret->active = false;
	ret->break_immediatly = break_immediatly;
	ret->break_depth = -1;
	ret->stack_depth = get_stack_depth(L);
	ret->list_line = 0;
	ret->last_line = 0;
	ret->last_source = NULL;
	ret->last_command = NULL;
	ret->user = NULL;

	lua_newtable(L);
	ret->breakpoints = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushpdebugger(L, ret);
	lua_setfield(L, LUA_REGISTRYINDEX, "__debugger");

	luadebug_debugger_activate(ret);

	return ret;
}

static void luadebug_debugger_deactivate(struct luadebug_debugger *session, bool release)
{
	if (session->active) {
		struct lua_state *state = lua_state_get(session->top_L);
		assert(state);

		lua_state_setdebugger_hook(state, NULL);

#if HAKA_LUAJIT
		luaJIT_setmode(session->top_L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON);
#endif

		if (release) {
			lua_pushnil(session->top_L);
			lua_setfield(session->top_L, LUA_REGISTRYINDEX, "__debugger");
		}

		atomic_dec(&running_debugger);

		session->active = false;

		LOG_INFO(MODULE, "lua debugger deactivated");
	}
}

void luadebug_debugger_cleanup(struct luadebug_debugger *session)
{
	luadebug_debugger_deactivate(session, true);

	luaL_unref(session->top_L, LUA_REGISTRYINDEX, session->breakpoints);

	free(session->last_command);
	free(session);
}

void luadebug_debugger_user(struct luadebug_user *user)
{
	mutex_lock(&current_user_mutex);

	luadebug_user_release(&current_user);

	if (user) {
		current_user = user;
		luadebug_user_addref(user);
	}

	mutex_unlock(&current_user_mutex);
}

bool luadebug_debugger_start(struct lua_State *L, bool break_immediatly)
{
	struct luadebug_debugger *debugger = luadebug_debugger_get(L);
	if (!debugger) {
		if (!luadebug_debugger_create(L, break_immediatly)) {
			return false;
		}
	}
	else {
		luadebug_debugger_activate(debugger);
		debugger->break_immediatly = break_immediatly;
	}

	return true;
}

void luadebug_debugger_stop(struct lua_State *L)
{
	struct luadebug_debugger *debugger = luadebug_debugger_get(L);
	if (debugger) {
		luadebug_debugger_deactivate(debugger, false);
	}
}

bool luadebug_debugger_break(struct lua_State *L)
{
	struct luadebug_debugger *debugger = luadebug_debugger_get(L);
	if (debugger && debugger->active) {
		debugger->break_immediatly = true;
		debugger->break_depth = -1;
		return true;
	}
	return false;
}

bool luadebug_debugger_interrupt(struct lua_State *L, const char *reason)
{
	struct luadebug_debugger *debugger = luadebug_debugger_get(L);
	if (debugger && debugger->active) {
		lua_Debug ar;
		lua_getstack(debugger->L, 0, &ar);
		enter_debugger(debugger, &ar, reason, true);
		return true;
	}
	return false;
}

bool luadebug_debugger_breakall()
{
	if (atomic_get(&running_debugger) > 0 && !current_session) {
		if (atomic_get(&break_required)) {
			return false;
		}

		atomic_set(&break_required, 1);
		return true;
	}
	else {
		return false;
	}
}

bool luadebug_debugger_shutdown()
{
	atomic_set(&break_required, 0);
	return true;
}

void luadebug_debbugger_error_hook(struct lua_State *L)
{
	const char *error = lua_tostring (L, -1);
	luadebug_debugger_interrupt(L, error);
}

INIT void _luadebug_debugger_init()
{
	mutex_init(&current_user_mutex, true);
	lua_state_error_hook = luadebug_debbugger_error_hook;
}

FINI void _luadebug_debugger_fini()
{
	lua_state_error_hook = NULL;
	luadebug_debugger_user(NULL);

	mutex_destroy(&current_user_mutex);
}
