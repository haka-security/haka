/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%typemap(in) (int ARGC, char **ARGV) {
	if (lua_istable(L,$input)) {
		int i, size = lua_objlen(L,$input);
		$1 = ($1_ltype) size;
		$2 = (char **) malloc((size+1)*sizeof(char *));
		for (i = 0; i < size; i++) {
			lua_rawgeti(L,$input,i+1);
			$2[i] = (char *)lua_tostring(L, -1);
			lua_pop(L,1);
		}
		$2[i]=NULL;
	} else {
		$1 = 0; $2 = 0;
		lua_pushstring(L,"Expecting argv array");
		SWIG_fail;
	}
}

%typemap(typecheck, precedence=SWIG_TYPECHECK_STRING_ARRAY) (int ARGC, char **ARGV) {
	$1 = lua_istable(L,$input);
}

%typemap(freearg) (int ARGC, char **ARGV) {
	free((char *) $2);
}
