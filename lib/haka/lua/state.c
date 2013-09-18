
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
#include <haka/log.h>
#include <haka/compiler.h>
#include <luadebug/debugger.h>


#define STATE_TABLE "__haka_state"



struct lua_state_ext {
	struct lua_state       state;
	struct lua_state_ext  *next;
};


static int panic(lua_State *L)
{
	messagef(HAKA_LOG_FATAL, L"lua", L"lua panic: %s", lua_tostring(L, -1));
	raise(SIGTERM);
	return 0;
}

void lua_state_print_error(struct lua_State *L, const wchar_t *msg)
{
	if (msg)
		messagef(HAKA_LOG_ERROR, L"lua", L"%ls: %s", msg, lua_tostring(L, -1));
	else
		messagef(HAKA_LOG_ERROR, L"lua", L"%s", lua_tostring(L, -1));

	lua_pop(L, 1);
}

void (*lua_state_error_hook)(struct lua_State *L) = NULL;

int lua_state_error_formater(lua_State *L)
{
	if (lua_state_error_hook) {
		lua_state_error_hook(L);
	}

	if (getlevel(L"lua") >= HAKA_LOG_DEBUG) {
		if (!lua_isstring(L, -1)) {
			return 0;
		}

		lua_getfield(L, LUA_GLOBALSINDEX, "debug");
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


/*
 * Redefine the luaL_tolstring (taken from Lua 5.2)
 */
const char *lua_converttostring(struct lua_State *L, int idx, size_t *len)
{
	if (!luaL_callmeta(L, idx, "__tostring")) {  /* no metafield? */
		switch (lua_type(L, idx)) {
			case LUA_TNUMBER:
			case LUA_TSTRING:
				lua_pushvalue(L, idx);
				break;
			case LUA_TBOOLEAN:
				lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
				break;
			case LUA_TNIL:
				lua_pushliteral(L, "nil");
				break;
			default:
				lua_pushfstring(L, "%s: %p", luaL_typename(L, idx),
						lua_topointer(L, idx));
				break;
		}
	}
	return lua_tolstring(L, -1, len);
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

#if !HAKA_LUAJIT

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
	ret->next = NULL;

	lua_atpanic(L, panic);

	luaL_openlibs(L);

	lua_pushcfunction(L, lua_print);
	lua_setglobal(L, "print");

#if !HAKA_LUAJIT
	lua_getglobal(L, "string");
	lua_pushcfunction(L, str_format);
	lua_setfield(L, -2, "format");
#endif

	lua_getglobal(L, "debug");
	lua_pushcfunction(L, lua_state_error_formater);
	lua_setfield(L, -2, "format_error");

	lua_object_initialize(L);

	lua_pushlightuserdata(L, ret);
	lua_setfield(L, LUA_REGISTRYINDEX, STATE_TABLE);

	ret->next = allocated_state;
	allocated_state = ret;

	return &ret->state;
}

void lua_state_close(struct lua_state *state)
{
	lua_close(state->L);
	state->L = NULL;
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

struct lua_state *lua_state_get(lua_State *L)
{
	const void *p;

	lua_getfield(L, LUA_REGISTRYINDEX, STATE_TABLE);
	assert(lua_islightuserdata(L, -1));
	p = lua_topointer(L, -1);
	lua_pop(L, 1);

	return (struct lua_state *)p;
}
