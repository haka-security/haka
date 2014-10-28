/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <haka/lua/state.h>
#include <haka/lua/object.h>
#include <haka/lua/ref.h>
#include <haka/log.h>
#include <haka/compiler.h>
#include <haka/error.h>
#include <haka/timer.h>
#include <haka/lua/luautils.h>
#include <haka/container/vector.h>
#include <haka/luadebug/debugger.h>


#define STATE_TABLE      "__haka_state"


struct lua_interrupt_data {
	lua_function          function;
	void                 *data;
	void                (*destroy)(void *);
};

struct lua_state_ext {
	struct lua_state       state;
	bool                   hook_installed;
	lua_hook               debug_hook;
	struct vector          interrupts;
	bool                   has_interrupts;
	struct lua_state_ext  *next;
};

static void lua_dispatcher_hook(lua_State *L, lua_Debug *ar);


static int panic(lua_State *L)
{
	LOG_FATAL(lua, "lua panic: %s", lua_tostring(L, -1));
	raise(SIGQUIT);
	return 0;
}

void lua_state_print_error(struct lua_State *L, const char *msg)
{
	if (msg) LOG_ERROR(lua, "%s: %s", msg, lua_tostring(L, -1));
	else LOG_ERROR(lua, "%s", lua_tostring(L, -1));

	lua_pop(L, 1);
}

void (*lua_state_error_hook)(struct lua_State *L) = NULL;

int lua_state_error_formater(lua_State *L)
{
	if (lua_state_error_hook && !lua_isnil(L, -1)) {
		lua_state_error_hook(L);
	}

	if (SHOULD_LOG_DEBUG(lua)) {
		if (!lua_isstring(L, -1)) {
			return 0;
		}

		lua_getglobal(L, "debug");
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			return 0;
		}

		lua_getfield(L, -1, "traceback");
		if (!lua_isfunction(L, -1)) {
			lua_pop(L, 2);
			return 0;
		}

		lua_pushvalue(L, -3);
		lua_pushinteger(L, 2);
		lua_call(L, 2, 1);

		return 1;
	}
	else {
		lua_pushvalue(L, -1);
		return 1;
	}
}

static int lua_casttopointer(lua_State* L)
{
	lua_pushfstring(L, "%p", lua_topointer(L, 1));
	return 1;
}

static int lua_print(lua_State* L)
{
	int i;
	int nargs = lua_gettop(L);

	for (i=1; i<=nargs; i++) {
		if (i > 1)
			printf(" ");

		printf("%s", lua_converttostring(L, i, NULL));
		lua_pop(L, 1);
	}

	printf("\n");

	return 0;
}

#if !HAKA_LUAJIT && !HAKA_LUA52

/*
 * On Lua 5.1, the string.format function does not convert userdata to
 * string automatically. We then need to redefine it (code taken from Lua 5.2)
 */
#define L_ESC            '%'
#define LUA_FLTFRMLEN    ""
#define LUA_FLTFRM_T     double
#define MAX_ITEM         512
#define FLAGS            "-+ #0"
#define MAX_FORMAT       (sizeof(FLAGS) + sizeof(LUA_INTFRMLEN) + 10)
#define uchar(c)         ((unsigned char)(c))

static void addquoted(lua_State *L, luaL_Buffer *b, int arg)
{
	size_t l;
	const char *s = luaL_checklstring(L, arg, &l);
	luaL_addchar(b, '"');
	while (l--) {
		if (*s == '"' || *s == '\\' || *s == '\n') {
			luaL_addchar(b, '\\');
			luaL_addchar(b, *s);
		}
		else if (*s == '\0' || iscntrl(uchar(*s))) {
			char buff[10];
			if (!isdigit(uchar(*(s+1))))
				sprintf(buff, "\\%d", (int)uchar(*s));
			else
				sprintf(buff, "\\%03d", (int)uchar(*s));
			luaL_addstring(b, buff);
		}
		else
			luaL_addchar(b, *s);
		s++;
	}
	luaL_addchar(b, '"');
}

static const char *scanformat(lua_State *L, const char *strfrmt, char *form)
{
	const char *p = strfrmt;
	while (*p != '\0' && strchr(FLAGS, *p) != NULL) p++;  /* skip flags */
	if ((size_t)(p - strfrmt) >= sizeof(FLAGS)/sizeof(char))
		luaL_error(L, "invalid format (repeated flags)");
	if (isdigit(uchar(*p))) p++;  /* skip width */
	if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
	if (*p == '.') {
		p++;
		if (isdigit(uchar(*p))) p++;  /* skip precision */
		if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
	}
	if (isdigit(uchar(*p)))
		luaL_error(L, "invalid format (width or precision too long)");
	*(form++) = '%';
	memcpy(form, strfrmt, (p - strfrmt + 1) * sizeof(char));
	form += p - strfrmt + 1;
	*form = '\0';
	return p;
}

static void addlenmod(char *form, const char *lenmod)
{
	size_t l = strlen(form);
	size_t lm = strlen(lenmod);
	char spec = form[l - 1];
	strcpy(form + l - 1, lenmod);
	form[l + lm - 1] = spec;
	form[l + lm] = '\0';
}

static int str_format(lua_State *L)
{
	int top = lua_gettop(L);
	int arg = 1;
	size_t sfl;
	const char *strfrmt = luaL_checklstring(L, arg, &sfl);
	const char *strfrmt_end = strfrmt+sfl;
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	while (strfrmt < strfrmt_end) {
		if (*strfrmt != L_ESC)
			luaL_addchar(&b, *strfrmt++);
		else if (*++strfrmt == L_ESC)
			luaL_addchar(&b, *strfrmt++);  /* %% */
		else { /* format item */
			char form[MAX_FORMAT];  /* to store the format (`%...') */
			char *buff = luaL_prepbuffer(&b);  /* to put formatted item */
			int nb = 0;  /* number of bytes in added item */
			if (++arg > top)
				luaL_argerror(L, arg, "no value");
			strfrmt = scanformat(L, strfrmt, form);
			switch (*strfrmt++) {
				case 'c':
					{
						nb = sprintf(buff, form, luaL_checkint(L, arg));
						break;
					}
				case 'd': case 'i':
					{
						lua_Number n = luaL_checknumber(L, arg);
						LUA_INTFRM_T ni = (LUA_INTFRM_T)n;
						lua_Number diff = n - (lua_Number)ni;
						luaL_argcheck(L, -1 < diff && diff < 1, arg,
								"not a number in proper range");
						addlenmod(form, LUA_INTFRMLEN);
						nb = sprintf(buff, form, ni);
						break;
					}
				case 'o': case 'u': case 'x': case 'X':
					{
						lua_Number n = luaL_checknumber(L, arg);
						unsigned LUA_INTFRM_T ni = (unsigned LUA_INTFRM_T)n;
						lua_Number diff = n - (lua_Number)ni;
						luaL_argcheck(L, -1 < diff && diff < 1, arg,
								"not a non-negative number in proper range");
						addlenmod(form, LUA_INTFRMLEN);
						nb = sprintf(buff, form, ni);
						break;
					}
				case 'e': case 'E': case 'f':
#if defined(LUA_USE_AFORMAT)
				case 'a': case 'A':
#endif
				case 'g': case 'G':
					{
						addlenmod(form, LUA_FLTFRMLEN);
						nb = sprintf(buff, form, (LUA_FLTFRM_T)luaL_checknumber(L, arg));
						break;
					}
				case 'q':
					{
						addquoted(L, &b, arg);
						break;
					}
				case 's':
					{
						size_t l;
						const char *s = lua_converttostring(L, arg, &l);
						if (!strchr(form, '.') && l >= 100) {
							/* no precision and string is too long to be formatted;
							   keep original string */
							luaL_addvalue(&b);
							break;
						}
						else {
							nb = sprintf(buff, form, s);
							lua_pop(L, 1);  /* remove result from 'lua_converttostring' */
							break;
						}
					}
				default:
					{  /* also treat cases `pnLlh' */
						return luaL_error(L, "invalid option " LUA_QL("%%%c") " to "
								LUA_QL("format"), *(strfrmt - 1));
					}
			}
			luaL_addsize(&b, nb);
		}
	}
	luaL_pushresult(&b);
	return 1;
}

#endif

static struct lua_state_ext *allocated_state = NULL;

static void lua_interrupt_data_destroy(void *_data)
{
	struct lua_interrupt_data *data = (struct lua_interrupt_data *)_data;
	if (data->destroy) {
		data->destroy(data->data);
	}
}

void lua_state_load_module(struct lua_state *state, lua_CFunction luaopen, const char *name)
{
	lua_pushcfunction(state->L, luaopen);
	lua_call(state->L, 0, 1);
	if (name) lua_setglobal(state->L, name);
	else lua_pop(state->L, 1);
}

#if HAKA_LUA52
void lua_getfenv(struct lua_State *L, int index)
{
	lua_getupvalue(L, index, 1);
}

int  lua_setfenv(struct lua_State *L, int index)
{
	return lua_setupvalue(L, index, 1) == NULL ? 0 : 1;
}

static int lua_getfenv_wrapper(lua_State *L)
{
	lua_getfenv(L, 1);
	return 1;
}

static int lua_setfenv_wrapper(lua_State *L)
{
	int ret;

	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	ret = lua_setfenv(L, -2);
	lua_pop(L, 1);
	return ret;
}
#endif

struct lua_state *lua_state_init()
{
	struct lua_state_ext *ret;
	lua_State *L = luaL_newstate();
	if (!L) {
		return NULL;
	}

	ret = malloc(sizeof(struct lua_state_ext));
	if (!ret) {
		return NULL;
	}

	ret->state.L = L;
	ret->hook_installed = false;
	ret->debug_hook = NULL;
	ret->has_interrupts = false;
	vector_create_reserve(&ret->interrupts, struct lua_interrupt_data, 20, lua_interrupt_data_destroy);
	ret->next = NULL;

	lua_atpanic(L, panic);

	luaL_openlibs(L);

	lua_pushcfunction(L, lua_print);
	lua_setglobal(L, "print");

	lua_pushcfunction(L, lua_casttopointer);
	lua_setglobal(L, "topointer");

#if !HAKA_LUAJIT && !HAKA_LUA52
	lua_getglobal(L, "string");
	lua_pushcfunction(L, str_format);
	lua_setfield(L, -2, "format");
#endif

#if HAKA_LUA52
	lua_getglobal(L, "debug");
	lua_pushcfunction(L, lua_getfenv_wrapper);
	lua_setfield(L, -2, "getfenv");
	lua_pushcfunction(L, lua_setfenv_wrapper);
	lua_setfield(L, -2, "setfenv");
	lua_pop(L, 1);
#endif

	lua_getglobal(L, "debug");
	lua_pushcfunction(L, lua_state_error_formater);
	lua_setfield(L, -2, "format_error");

	lua_object_initialize(L);

	lua_pushlightuserdata(L, ret);
	lua_setfield(L, LUA_REGISTRYINDEX, STATE_TABLE);

	lua_ref_init_state(L);

	ret->next = allocated_state;
	allocated_state = ret;

	return &ret->state;
}

void lua_state_trigger_haka_event(struct lua_state *state, const char *event)
{
	int h;
	LUA_STACK_MARK(state->L);

	lua_pushcfunction(state->L, lua_state_error_formater);
	h = lua_gettop(state->L);

	/*
	 * haka.context:signal(nil, haka.events[event])
	 */
	lua_getglobal(state->L, "haka");
	lua_getfield(state->L, -1, "events");
	lua_getfield(state->L, -2, "context");
	lua_getfield(state->L, -1, "signal"); /* function haka.context.signal */
	lua_pushvalue(state->L, -2);          /* self */
	lua_pushnil(state->L);                /* emitter */
	lua_getfield(state->L, -5, event);    /* event */
	if (lua_isnil(state->L, -1)) {
		LOG_ERROR(lua, "invalid haka event: %s", event);
	}
	else {
		if (lua_pcall(state->L, 3, 0, h)) {
			lua_state_print_error(state->L, "lua");
		}
	}

	lua_pop(state->L, 4);

	LUA_STACK_CHECK(state->L, 0);
}

void lua_state_close(struct lua_state *_state)
{
	struct lua_state_ext *state = (struct lua_state_ext *)_state;

	LOG_DEBUG(lua, "closing state");

	lua_state_trigger_haka_event(_state, "exiting");

	vector_destroy(&state->interrupts);
	state->has_interrupts = false;

	lua_close(state->state.L);
	state->state.L = NULL;
}

FINI_P(2000) static void lua_state_cleanup()
{
	struct lua_state_ext *del;
	struct lua_state_ext *ptr = allocated_state;
	while (ptr) {
		del = ptr;
		ptr = ptr->next;

		free(del);
	}

	allocated_state = NULL;
}

bool lua_state_isvalid(struct lua_state *state)
{
	return (state->L != NULL);
}

static struct lua_state_ext *lua_state_getext(lua_State *L)
{
	const void *p;

	lua_getfield(L, LUA_REGISTRYINDEX, STATE_TABLE);
	assert(lua_islightuserdata(L, -1));
	p = lua_topointer(L, -1);
	lua_pop(L, 1);

	return (struct lua_state_ext *)p;
}

struct lua_state *lua_state_get(lua_State *L)
{
	return &lua_state_getext(L)->state;
}

static void lua_interrupt_call(struct lua_state_ext *state)
{
	int i, h;
	LUA_STACK_MARK(state->state.L);
	struct vector interrupts;

	vector_create_reserve(&interrupts, struct lua_interrupt_data, 20, lua_interrupt_data_destroy);

	vector_swap(&state->interrupts, &interrupts);
	state->has_interrupts = false;

	lua_pushcfunction(state->state.L, lua_state_error_formater);
	h = lua_gettop(state->state.L);

	for (i=0; i<vector_count(&interrupts); ++i) {
		struct lua_interrupt_data *func = vector_get(&interrupts, struct lua_interrupt_data, i);
		assert(func);

		lua_pushcfunction(state->state.L, func->function);

		if (func->data) {
			lua_pushlightuserdata(state->state.L, func->data);
		}

		if (lua_pcall(state->state.L, func->data ? 1 : 0, 0, h)) {
			if (!lua_isnil(state->state.L, -1)) {
				lua_state_print_error(state->state.L, "lua");
			}
			else {
				lua_pop(state->state.L, 1);
			}
		}
	}

	vector_destroy(&interrupts);

	lua_pop(state->state.L, 1);
	LUA_STACK_CHECK(state->state.L, 0);
}

static void lua_update_hook(struct lua_state_ext *state)
{
	if (state->debug_hook || state->has_interrupts) {
		if (!state->hook_installed) {
			lua_sethook(state->state.L, &lua_dispatcher_hook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 1);
			state->hook_installed = true;
		}
	}
	else {
		if (state->hook_installed) {
			lua_sethook(state->state.L, &lua_dispatcher_hook, 0, 1);
			state->hook_installed = false;
		}
	}
}

static void lua_dispatcher_hook(lua_State *L, lua_Debug *ar)
{
	struct lua_state_ext *state = lua_state_getext(L);
	if (state) {
		if (state->debug_hook) {
			state->debug_hook(L, ar);
		}

		if (state->has_interrupts)
		{
			lua_interrupt_call(state);
			lua_update_hook(state);
		}
	}
}

bool lua_state_setdebugger_hook(struct lua_state *_state, lua_hook hook)
{
	struct lua_state_ext *state = (struct lua_state_ext *)_state;

	state->debug_hook = hook;
	lua_update_hook(state);

	return true;
}

bool lua_state_interrupt(struct lua_state *_state, lua_function func, void *data, void (*destroy)(void *))
{
	struct lua_state_ext *state = (struct lua_state_ext *)_state;
	struct lua_interrupt_data *func_data;

	if (!lua_state_isvalid(&state->state)) {
		error("invalid lua state");
		return false;
	}

	assert(func);
	func_data = vector_push(&state->interrupts, struct lua_interrupt_data);
	func_data->function = func;
	func_data->data = data;
	func_data->destroy = destroy;

	state->has_interrupts = true;
	lua_update_hook(state);

	return true;
}

bool lua_state_has_interrupts(struct lua_state *_state)
{
	struct lua_state_ext *state = (struct lua_state_ext *)_state;

	return state->has_interrupts;
}

int lua_state_runinterrupt(lua_State *L)
{
	struct lua_state_ext *state;

	LUA_STACK_MARK(L);

	assert(lua_islightuserdata(L, -1));
	state = lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (!vector_isempty(&state->interrupts)) {
		state->has_interrupts = false;
		lua_update_hook(state);

		lua_interrupt_call(state);
	}

	LUA_STACK_CHECK(L, 0);

	return 0;
}

bool lua_state_run_file(struct lua_state *state, const char *filename, int argc, char *argv[])
{
	int i, h;

	lua_pushcfunction(state->L, lua_state_error_formater);
	h = lua_gettop(state->L);

	if (luaL_loadfile(state->L, filename)) {
		lua_state_print_error(state->L, NULL);
		lua_pop(state->L, 1);
		return false;
	}
	for (i = 1; i <= argc; i++) {
		lua_pushstring(state->L, argv[i-1]);
	}
	if (lua_pcall(state->L, argc, 0, h)) {
		lua_state_print_error(state->L, NULL);
		lua_pop(state->L, 1);
		return false;
	}

	lua_pop(state->L, 1);
	return true;
}

void lua_state_preload(struct lua_state *state, const char *module, lua_CFunction cfunc)
{
	LUA_STACK_MARK(state->L);

	lua_getglobal(state->L, "package");
	lua_pushstring(state->L, "preload");
	lua_gettable(state->L, -2);
	lua_pushcfunction(state->L, cfunc);
	lua_setfield(state->L, -2, module);

	lua_pop(state->L, 2);

	LUA_STACK_CHECK(state->L, 0);
}

bool lua_state_require(struct lua_state *state, const char *module)
{
	int h;

	lua_pushcfunction(state->L, lua_state_error_formater);
	h = lua_gettop(state->L);

	lua_getglobal(state->L, "require");
	lua_pushstring(state->L, module);

	if (lua_pcall(state->L, 1, 0, h)) {
		lua_state_print_error(state->L, NULL);
		lua_pop(state->L, 1);
		return false;
	}

	lua_pop(state->L, 1);
	return true;
}


/*
 * Debugging utility functions
 */
static void dump_frame(lua_State *L, lua_Debug *ar)
{
	lua_getinfo(L, "Snl", ar);

	if (!strcmp(ar->what, "C")) {
		printf("[C]: in function '%s'\n", ar->name);
	}
	else if (!strcmp(ar->what, "main")) {
		printf("%s:%d: in the main chunk\n",
				ar->short_src, ar->currentline);
	}
	else if (!strcmp(ar->what, "Lua")) {
		printf("%s:%d: in function '%s'\n",
				ar->short_src, ar->currentline, ar->name);
	}
	else if (!strcmp(ar->what, "tail")) {
		printf("in tail call\n");
	}
	else {
		printf("%s\n", ar->what);
	}
}

void lua_state_dumpbacktrace(lua_State *L)
{
	int i;
	lua_Debug ar;

	for (i = 0; lua_getstack(L, i, &ar); ++i) {
		printf("  #%i\t", i);
		dump_frame(L, &ar);
	}
}
