/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module hakainit

%{
#include <stdint.h>
#include <unistd.h>
#include <wchar.h>

#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/module.h>
#include <haka/alert.h>
#include <haka/config.h>
#include <haka/colors.h>
#include <haka/engine.h>
#include <haka/system.h>

char *module_prefix = HAKA_MODULE_PREFIX;
char *module_suffix = HAKA_MODULE_SUFFIX;

static bool stdout_support_colors()
{
	return colors_supported(STDOUT_FILENO);
}

%}

%include "haka/lua/swig.si"

%nodefaultctor;
%nodefaultdtor;

%rename(module_path) module_get_path;
const char *module_get_path(bool c);

%immutable;
char *module_prefix;
char *module_suffix;
%mutable;

bool stdout_support_colors();

%luacode {
	hakainit = unpack({...})

	local function string_split(str, delim)
		local ret = {}
		local last_end = 1
		local s, e = str:find(delim, 1)

		while s do
			cap = str:sub(last_end, e-1)
			table.insert(ret, cap)

			last_end = e+1
			s, e = str:find(delim, last_end)
		end

		if last_end <= #str then
			cap = str:sub(last_end)
			table.insert(ret, cap)
		end

		return ret
	end

	local function addpath(dst, paths, exts)
		if not paths then return dst end

		local pathtable = {}

		for _, path in pairs(string_split(paths, ';')) do
			for _, ext in pairs(exts) do
				local p = string.gsub(path, '*', ext)
				table.insert(pathtable, p)
			end
		end

		table.insert(pathtable, dst)

		return table.concat(pathtable, ';')
	end

	package.cpath = addpath(package.cpath, hakainit.module_path(true), { hakainit.module_prefix .. '?' .. hakainit.module_suffix })
	package.path = addpath(package.path, hakainit.module_path(false), { '?.bc', '?.lua' })
}
