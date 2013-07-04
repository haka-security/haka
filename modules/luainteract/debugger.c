
#define _GNU_SOURCE

#include <haka/error.h>
#include <haka/compiler.h>
#include <haka/thread.h>
#include <haka/lua/state.h>
#include <haka/lua/lua.h>

#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>

#ifdef HAKA_LUAJIT
#include <luajit.h>
#endif

#include "debugger.h"
#include "complete.h"
#include "utils.h"
#include "readline.h"


struct luainteract_debugger {
	lua_State                   *top_L;
	lua_State                   *L;
	struct luainteract_complete  complete;

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
};

static mutex_t active_session_mutex = MUTEX_INIT;
static struct luainteract_debugger *current_session;
#define LIST_DEFAULT_LINE   10


static char *complete_callback_debug_keyword(struct lua_State *L, struct luainteract_complete *context,
		const char *text, int state)
{
	static const char *keywords[] = {
		"bt", "stack", "local", "upvalue", "list", "n", "next", "l", "frame", "f",
		"continue", "c", "removebreak", "rb", "print", "p", "step", "s",
		"finish", "listbreak", "lb", NULL
	};

	return complete_keyword(context, keywords, text, state);
}

static const complete_callback completions[] = {
	&complete_callback_debug_keyword,
	NULL
};

static char *empty_generator(const char *text, int state)
{
	return NULL;
}

static char *command_generator(const char *text, int state)
{
	char *match = complete_generator(current_session->L, &current_session->complete,
			completions, text, state);
	rl_completion_suppress_append = !current_session->complete.space;
	return match;
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
	char *match = complete_generator(current_session->L, &current_session->complete,
			lua_completions, text, state);
	rl_completion_suppress_append = !current_session->complete.space;
	return match;
}

static char **complete(const char *text, int start, int end)
{
	current_session->complete.stack_env = -1;
	if (start == 0) {
		return rl_completion_matches(text, command_generator);
	}
	else {
		if (strncmp(rl_line_buffer, "p ", 2) == 0 ||
			strncmp(rl_line_buffer, "print ", 6) == 0) {
			current_session->complete.stack_env = current_session->env_index;
			return rl_completion_matches(text, lua_generator);
		}
		else {
			return NULL;
		}
	}
}

static void dump_frame(lua_State *L, lua_Debug *ar)
{
	lua_getinfo(L, "Snl", ar);

	if (!strcmp(ar->what, "C")) {
		printf("[C]" CLEAR ": in function '" MAGENTA "%s" CLEAR "'\n",
				ar->name);
	}
	else if (!strcmp(ar->what, "main")) {
		printf("%s:%d" CLEAR ": in the main chunk\n",
				ar->short_src, ar->currentline);
	}
	else if (!strcmp(ar->what, "Lua")) {
		printf("%s:%d" CLEAR ": in function '" MAGENTA "%s" CLEAR "'\n",
				ar->short_src, ar->currentline, ar->name);
	}
	else if (!strcmp(ar->what, "tail")) {
		printf(CLEAR "in tail call\n");
	}
	else {
		printf("%s\n" CLEAR, ar->what);
	}
}

static void dump_backtrace(lua_State *L, int current)
{
	int i;
	lua_Debug ar;

	printf("Backtrace\n");

	for (i = 0; lua_getstack(L, i, &ar); ++i) {
		if (i == current) {
			printf(RED BOLD " =>" CLEAR "%d\t" CYAN, i);
		}
		else {
			printf("  #%d\t" CYAN, i);
		}

		dump_frame(L, &ar);
	}
}

static int get_stack_depth(lua_State *L)
{
	int i;
	lua_Debug ar;

	for (i = 0; lua_getstack(L, i, &ar); ++i);
	return i;
}

static void change_frame(struct luainteract_debugger *session, int index)
{
	if (!lua_getstack(session->L, index, &session->frame)) {
		printf(RED "invalid frame index '%d'\n" CLEAR, index);
	}
	else {
		int env_index;

		printf("  #%d\t" CYAN, index);
		dump_frame(session->L, &session->frame);

		session->frame_index = index;

		env_index = capture_env(session->L, index);
		assert(env_index != -1);

		lua_replace(session->L, session->env_index);

		session->list_line = session->frame.currentline - LIST_DEFAULT_LINE/2;
	}
}

static void dump_stack(lua_State *L, int index)
{
	int i, h;

	h = lua_gettop(L);

	lua_getglobal(L, "luainteract");
	lua_getfield(L, -1, "pprint");

	if (index < 0) {
		printf("Stack (size=%d)\n", h-1);
		for (i = 1; i < h; ++i) {
			printf("  #%d\t", i);

			lua_pushvalue(L, -1);
			lua_pushvalue(L, i);
			lua_pushstring(L, "    \t");
			lua_pushnumber(L, 1);
			lua_call(L, 3, 0);
		}
	}
	else if (index > 0 && index < h) {
		printf("  #%d\t", index);

		lua_pushvalue(L, -1);
		lua_pushvalue(L, index);
		lua_pushstring(L, "    \t");
		lua_call(L, 2, 0);
	}
	else {
		printf(RED "invalid stack index '%d'\n" CLEAR, index);
	}

	lua_pop(L, 2);
}

static void dump_local(lua_State *L, lua_Debug *ar, int index)
{
	int i;
	const char *local;

	lua_getglobal(L, "luainteract");
	lua_getfield(L, -1, "pprint");

	if (index < 0) {
		printf("Locals\n");
		for (i=1; (local = lua_getlocal(L, ar, i)); ++i) {
			printf("  #%d\t%s = ", i, local);

			lua_pushvalue(L, -2);
			lua_pushvalue(L, -2);
			lua_pushstring(L, "    \t");
			lua_pushnumber(L, 1);
			lua_call(L, 3, 0);

			lua_pop(L, 1);
		}
	}
	else {
		local = lua_getlocal(L, ar, index);
		if (!local) {
			printf(RED "invalid local index '%d'\n" CLEAR, index);
		}
		else {
			printf("  #%d\t%s = ", index, local);

			lua_pushvalue(L, -2);
			lua_pushvalue(L, -2);
			lua_pushstring(L, "    \t");
			lua_call(L, 2, 0);

			lua_pop(L, 1);
		}
	}

	lua_pop(L, 2);
}

static void dump_upvalue(lua_State *L, lua_Debug *ar, int index)
{
	int i;
	const char *local;

	lua_getinfo(L, "f", ar);
	lua_getglobal(L, "luainteract");
	lua_getfield(L, -1, "pprint");

	if (index < 0) {
		printf("Up-values\n");
		for (i=1; (local = lua_getupvalue(L, -3, i)); ++i) {
			printf("  #%d\t%s = ", i, local);

			lua_pushvalue(L, -2);
			lua_pushvalue(L, -2);
			lua_pushstring(L, "    \t");
			lua_pushnumber(L, 1);
			lua_call(L, 3, 0);

			lua_pop(L, 1);
		}
	}
	else {
		local = lua_getupvalue(L, -3, index);
		if (!local) {
			printf(RED "invalid up-value index '%d'\n" CLEAR, index);
		}
		else {
			printf("  #%d\t%s = ", index, local);

			lua_pushvalue(L, -2);
			lua_pushvalue(L, -2);
			lua_pushstring(L, "    \t");
			lua_call(L, 2, 0);

			lua_pop(L, 1);
		}
	}

	lua_pop(L, 3);
}

static bool push_breapoints(struct luainteract_debugger *session, lua_Debug *ar)
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

static bool has_breakpoint(struct luainteract_debugger *session, lua_Debug *ar)
{
	bool breakpoint = false;

	if (push_breapoints(session, ar)) {
		lua_pushnumber(session->L, ar->currentline);
		lua_gettable(session->L, -2);
		breakpoint = !lua_isnil(session->L, -1);
		lua_pop(session->L, 2);
	}

	return breakpoint;
}

static void add_breakpoint(struct luainteract_debugger *session, int line)
{
	if (push_breapoints(session, &session->frame)) {
		lua_pushnumber(session->L, line);
		lua_pushboolean(session->L, true);
		lua_settable(session->L, -3);
		lua_pop(session->L, 1);

		printf("breakpoint added at '%s:%d'\n", session->frame.short_src, line);
	}
	else {
		printf(RED "invalid breakpoint location\n" CLEAR);
	}
}

static void remove_breakpoint(struct luainteract_debugger *session, int line)
{
	if (push_breapoints(session, &session->frame)) {
		lua_pushnumber(session->L, line);
		lua_gettable(session->L, -2);
		if (lua_isnil(session->L, -1)) {
			lua_pop(session->L, 1);
			printf(RED "not breakpoint at '%s:%d'\n" CLEAR, session->frame.short_src, line);
		}
		else {
			lua_pop(session->L, 1);
			lua_pushnumber(session->L, line);
			lua_pushnil(session->L);
			lua_settable(session->L, -3);
			printf("breakpoint removed from '%s:%d'\n", session->frame.short_src, line);
		}

		lua_pop(session->L, 1);
	}
	else {
		printf(RED "invalid breakpoint location\n" CLEAR);
	}
}

static void list_breakpoint(struct luainteract_debugger *session)
{
	//int i = 0;
}

static bool print_line_char(lua_State *L, int c, int line, int current, int *count,
		unsigned int start, unsigned int end, bool show_breakpoints)
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
					printf(RED "%4d" BOLD "#> " CLEAR, line);
				}
				else {
					printf(RED "%4d" BOLD "=> " CLEAR, line);
				}
			}
			else {
				if (breakpoint) {
					printf(YELLOW "%4d" BOLD "#  " CLEAR, line);
				}
				else {
					printf(YELLOW "%4d:  " CLEAR, line);
				}
			}
		}
	}

	if (line >= start && line <= end) {
		if (c == '\t') printf("    ");
		else fputc(c, stdout);
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
		unsigned int start, unsigned int end, bool show_breakpoints)
{
	int c;
	int count = 0;

	while ((c = fgetc(file)) > 0) {
		if (print_line_char(L, c, num, current, &count, start, end, show_breakpoints)) {
			return true;
		}
	}

	if (count) {
		printf("\n");
	}

	return false;
}

static bool print_line(lua_State *L, const char **string, int num, int current,
		unsigned int start, unsigned int end, bool show_breakpoints)
{
	int c;
	int count = 0;

	while ((c = *(*string++)) > 0) {
		if (print_line_char(L, c, num, current, &count, start, end, show_breakpoints)) {
			return true;
		}
	}

	if (count) {
		printf("\n");
	}

	return false;
}

static void dump_source(struct luainteract_debugger *session, lua_Debug *ar, unsigned int start, unsigned int end)
{
	const bool breakpoints = push_breapoints(session, ar);

	lua_getinfo(session->L, "S", ar);

	if (ar->source[0] == '@') {
		int line = 1;
		FILE *file = fopen(ar->source+1, "r");

		while (print_file_line(session->L, file, line++, ar->currentline, start, end, breakpoints));

		fclose(file);
	}
	else if (ar->source[0] == '=') {
		printf("%s\n", ar->source+1);
	}
	else {
		int line = 1;
		const char *source = ar->source;

		while (print_line(session->L, &source, line++, ar->currentline, start, end, breakpoints));
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

static bool process_command(struct luainteract_debugger *session, const char *line,
		bool with_history)
{
	bool valid_line = true;
	bool cont = false;
	const char *option;

	if (check_token(line, "bt", NULL, &option) && !option) {
		dump_backtrace(session->L, session->frame_index);
	}
	else if (check_token(line, "frame", "f", &option)) {
		const int index = option ? atoi(option) : session->frame_index;
		change_frame(session, index);
	}
	else if (check_token(line, "stack", NULL, &option)) {
		const int index = option ? atoi(option) : -1;
		dump_stack(session->L, index);
	}
	else if (check_token(line, "local", NULL, &option)) {
		const int index = option ? atoi(option) : -1;
		dump_local(session->L, &session->frame, index);
	}
	else if (check_token(line, "upvalue", NULL, &option)) {
		const int index = option ? atoi(option) : -1;
		dump_upvalue(session->L, &session->frame, index);
	}
	else if (check_token(line, "print", "p", &option) && option) {
		int status;
		char *print_line = malloc(7 + strlen(option) + 1);

		strcpy(print_line, "return ");
		strcpy(print_line+7, option);

		status = luaL_loadbuffer(session->L, print_line, strlen(print_line), "stdin");
		if (status) {
			printf(RED "%s\n" CLEAR, lua_tostring(session->L, -1));
			lua_pop(session->L, 1);
		}
		else {
			lua_pushvalue(session->L, session->env_index);
			lua_setfenv(session->L, -2);

			execute_print(session->L);
			lua_pop(session->L, 1);
		}

		free(print_line);
	}
	else if (check_token(line, "list", "l", &option)) {
		if (option) {
			if (strcmp(option, "-") == 0) {
				session->list_line -= 2*LIST_DEFAULT_LINE;
			}
			else if (strcmp(option, "+") == 0) {
			}
			else {
				const int line = atoi(option);
				session->list_line = line - LIST_DEFAULT_LINE/2;
			}

			if (session->list_line < 0) {
				session->list_line = 0;
			}
		}

		dump_source(session, &session->frame, session->list_line, session->list_line + LIST_DEFAULT_LINE);
		session->list_line += LIST_DEFAULT_LINE;
	}
	else if (check_token(line, "break", "b", &option)) {
		const int line = option ? atoi(option) : session->frame.currentline;
		add_breakpoint(session, line);
	}
	else if (check_token(line, "removebreak", "rb", &option)) {
		const int line = option ? atoi(option) : session->frame.currentline;
		remove_breakpoint(session, line);
	}
	else if (check_token(line, "listbreak", "lb", &option) && !option) {
		list_breakpoint(session);
	}
	else if (check_token(line, "next", "n", &option) && !option) {
		session->break_immediatly = true;
		cont = true;
	}
	else if (check_token(line, "step", "s", &option) && !option) {
		session->break_immediatly = true;
		session->break_depth = session->stack_depth;
		cont = true;
	}
	else if (check_token(line, "finish", NULL, &option) && !option) {
		if (session->stack_depth > 0) {
			session->break_immediatly = true;
			session->break_depth = session->stack_depth-1;
		}
		cont = true;
	}
	else if (check_token(line, "continue", "c", &option) && !option) {
		cont = true;
	}
	else if (strlen(line) == 0) {
		HIST_ENTRY *history = previous_history();
		if (history && strlen(history->line) > 0) {
			cont = process_command(session, history->line, false);
		}

		valid_line = false;
	}
	else {
		printf(RED "invalid command '%s'\n" CLEAR, line);
		valid_line = false;
	}

	if (valid_line && with_history) {
		add_history(line);
	}

	return cont;
}

static void enter_debugger(struct luainteract_debugger *session, lua_Debug *ar)
{
	char *line;
	LUA_STACK_MARK(session->L);

	mutex_lock(&active_session_mutex);

	current_session = session;

	rl_basic_word_break_characters = " \t\n`@$><=;|&{(";
	rl_readline_name = "debug";
	rl_attempted_completion_function = complete;
	rl_completion_entry_function = empty_generator;
	rl_filename_completion_desired = 0;
	rl_completion_display_matches_hook = rl_display_matches;
	using_history();

	session->list_line = ar->currentline - LIST_DEFAULT_LINE/2;
	session->frame = *ar;
	session->frame_index = 0;
	session->env_index = capture_env(session->L, 0);
	assert(session->env_index != -1);
	session->break_immediatly = false;
	session->break_depth = -1;

	dump_source(session, ar, ar->currentline, ar->currentline);
	rl_on_new_line();

	while ((line = readline(GREEN "debug" BOLD ">  " CLEAR))) {
		const bool cont = process_command(session, line, true);
		free(line);

		if (cont) {
			break;
		}
	}

	if (!line) {
		printf("\n");
	}

	lua_pop(session->L, 1);

	current_session = NULL;
	LUA_STACK_CHECK(session->L, 0);

	rl_on_new_line();

	mutex_unlock(&active_session_mutex);
}

static void lua_debug_hook(lua_State *L, lua_Debug *ar)
{
	struct luainteract_debugger *session;

	LUA_STACK_MARK(L);

	lua_getfield(L, LUA_REGISTRYINDEX, "__debugger");
	assert(lua_islightuserdata(L, -1));
	session = (struct luainteract_debugger *)lua_topointer(L, -1);
	lua_pop(L, 1);
	assert(session);

	if (L != session->top_L) {
		return;
	}

	session->L = L;

	switch (ar->event) {
	case LUA_HOOKLINE:
		{
			lua_getinfo(session->L, "S", ar);
			session->stack_depth = get_stack_depth(L);

			if (session->break_immediatly && session->stack_depth <= session->break_depth) {
				enter_debugger(session, ar);
			}
			else if ((session->last_source != ar->source || session->last_line != ar->currentline) &&
					has_breakpoint(session, ar)) {
				enter_debugger(session, ar);
			}

			session->last_source = ar->source;
			session->last_line = ar->currentline;
		}
		break;

	case LUA_HOOKCALL:
		break;

	case LUA_HOOKRET:
	case LUA_HOOKTAILRET:
		break;

	default:
		break;
	}

	LUA_STACK_CHECK(L, 0);
}

struct luainteract_debugger *luainteract_debugger_create(struct lua_State *L)
{
	struct luainteract_debugger *ret;

	lua_getfield(L, LUA_REGISTRYINDEX, "__debugger");
	if (!lua_isnil(L, -1)) {
		error(L"debugger already attached");
		lua_pop(L, 1);
		return NULL;
	}
	lua_pop(L, 1);

	ret = malloc(sizeof(struct luainteract_debugger));
	if (!ret) {
		error(L"memory error");
		return NULL;
	}

	ret->top_L = L;
	ret->break_immediatly = true;
	ret->break_depth = -1;
	ret->stack_depth = get_stack_depth(L);
	ret->list_line = 0;
	ret->last_line = 0;
	ret->last_source = NULL;

	lua_pushlightuserdata(L, ret);
	lua_setfield(L, LUA_REGISTRYINDEX, "__debugger");

	lua_newtable(L);
	ret->breakpoints = luaL_ref(L, LUA_REGISTRYINDEX);

#ifdef HAKA_LUAJIT
	luaJIT_setmode(L, 0, LUAJIT_MODE_OFF);
#endif

	lua_sethook(L, &lua_debug_hook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 1);
	return ret;
}

void luainteract_debugger_cleanup(struct luainteract_debugger *session)
{
	lua_sethook(session->top_L, &lua_debug_hook, 0, 0);

	lua_pushnil(session->top_L);
	lua_setfield(session->top_L, LUA_REGISTRYINDEX, "__debugger");

	luaL_unref(session->top_L, LUA_REGISTRYINDEX, session->breakpoints);

	free(session);
}

void luainteract_debugger_break(struct luainteract_debugger *session)
{
	session->break_immediatly = true;
	session->break_depth = -1;
}
