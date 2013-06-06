%{
	#include <haka/error.h>

	#include <stdlib.h>

	typedef const char *temporary_string;
%}

%define DEFAULT_EXCEPTION

	const wchar_t *error;
	$action
	if ((error = clear_error())) {
		int size;
		char *errormb;

		size = wcstombs(NULL, error, 0);
		if (size == (size_t)-1) {
			lua_pushstring(L, "unknown error");
			SWIG_fail;
		}

		errormb = malloc(size+1);
		if (!errormb) {
			lua_pushstring(L, "memory error");
			SWIG_fail;
		}

		wcstombs(errormb, error, size+1);
		lua_pushstring(L, errormb);
		free(errormb);

		SWIG_fail;
	}
%enddef
%exception {
        DEFAULT_EXCEPTION
}

%typemap(out) temporary_string {
	lua_pushstring(L, (const char *)$result); SWIG_arg++;
	free((void*)$result);
}
