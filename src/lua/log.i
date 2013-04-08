%module log

%{
#include <haka/log.h>
#include <wchar.h>
#include <stdlib.h>

wchar_t* str2wstr(const char *str, int len)
{
	wchar_t* p;
	if (str==0 || len<1)  return 0;
	p=(wchar_t *)malloc((len+1)*sizeof(wchar_t));
	if (p==0) return 0;
	if (mbstowcs(p, str, len)==-1)
	{
		free(p);
		return 0;
	}
	p[len]=0;
	return p;
}
%}

%typemap(in, checkfn="SWIG_lua_isnilstring", fragment="SWIG_lua_isnilstring") wchar_t *
%{
	$1 = str2wstr(lua_tostring( L, $input ),lua_rawlen( L, $input ));
	if ($1==0) {lua_pushfstring(L,"Error in converting to wchar (arg %d)",$input);goto fail;}
%}

%typemap(freearg) wchar_t *
%{
	free($1);
%}

%typemap(typecheck) wchar_t * = char *;

%rename(FATAL) HAKA_LOG_FATAL;
%rename(ERROR) HAKA_LOG_ERROR;
%rename(WARNING) HAKA_LOG_WARNING;
%rename(INFO) HAKA_LOG_INFO;
%rename(DEBUG) HAKA_LOG_DEBUG;

enum log_level { HAKA_LOG_FATAL, HAKA_LOG_ERROR, HAKA_LOG_WARNING, HAKA_LOG_INFO, HAKA_LOG_DEBUG };

void message(log_level level, const wchar_t *module, const wchar_t *message);

%luacode {
	function log.messagef(level, module, fmt, ...)
		log.message(level, module, string.format(fmt, unpack(arg)))
	end
}

