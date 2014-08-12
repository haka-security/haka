/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module swig

%include "haka/lua/swig.si"

%nodefaultctor;
%nodefaultdtor;

%native(_getswigclassmetatable) int _getswigclassmetatable(struct lua_State *L);

%{
int _getswigclassmetatable(struct lua_State *L)
{
	SWIG_Lua_get_class_registry(L);
	return 1;
}
%}

%luacode {
	local this = unpack({...})

	function this.getclassmetatable(name)
		local ret = this._getswigclassmetatable()[name]
		assert(ret, string.format("unknown swig class '%s'", name))
		return ret
	end

	return this
}
