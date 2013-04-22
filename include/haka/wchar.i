
%{
#include <haka/log.h>
#include <wchar.h>
#include <stdlib.h>

static wchar_t* str2wstr(const char *str, int len)
{
	wchar_t *p;
	if (str == 0 || len < 1)
		return 0;

	p = (wchar_t *)malloc((len+1)*sizeof(wchar_t));
	if (p == 0)
		return 0;

	if (mbstowcs(p, str, len) == -1)
	{
		free(p);
		return 0;
	}

	p[len] = 0;
	return p;
}
%}

%typemap(in, checkfn="SWIG_lua_isnilstring", fragment="SWIG_lua_isnilstring") wchar_t *
%{
	$1 = str2wstr(lua_tostring(L, $input), lua_rawlen(L, $input));
	if ($1 == 0) { lua_pushfstring(L, "Error in converting to wchar (arg %d)", $input); goto fail; }
%}

%typemap(freearg) wchar_t *
%{
	free($1);
%}

%typemap(typecheck) wchar_t * = char *;
