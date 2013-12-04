/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module haka

%include "time.si"

%{
#include <stdint.h>
#include <wchar.h>

#include "app.h"
#include "state.h"
#include <haka/packet_module.h>
#include <haka/thread.h>
#include <haka/module.h>
#include <haka/stream.h>
#include <haka/alert.h>
#include <haka/config.h>

char *module_prefix = HAKA_MODULE_PREFIX;
char *module_suffix = HAKA_MODULE_SUFFIX;

struct time_lua *mk_lua_time(time_us ts)
{
	struct time_lua *t = malloc(sizeof(struct time_lua));
	if (!t) {
		error(L"memory error");
		return NULL;
	}

	t->seconds = ts / 1000000;
	t->micro_seconds = ts % 1000000;
	return t;
}

%}

%include "haka/lua/swig.si"
%include "haka/lua/stream.si"

%nodefaultctor;
%nodefaultdtor;

%rename(current_thread) thread_get_id;
int thread_get_id();

%rename(module_path) module_get_path;
const char *module_get_path();

%immutable;
char *module_prefix;
char *module_suffix;

struct stream {
};
BASIC_STREAM(stream)

%rename(time) time_lua;
struct time_lua {
	int    seconds;
	int    micro_seconds;

	%extend {
		time_lua() {
			return mk_lua_time(time_gettimestamp());
		}

		~time_lua() {
			free($self);
		}

		double toseconds()
		{
			return ((double)$self->seconds) + $self->micro_seconds / 1000000.;
		}

		temporary_string __tostring()
		{
			time_us ts;
			char *ret = malloc(TIME_BUFSIZE);
			if (!ret) {
				error(L"memory error");
				return NULL;
			}

			ts = $self->seconds*1000000LL + $self->micro_seconds;

			if (!time_tostring(ts, ret)) {
				assert(check_error());
				free(ret);
				return NULL;
			}

			return ret;
		}
	}
};

%luacode {
	haka = unpack({...})

	function string.split(str, delim)
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
		local pathtable = string.split(dst, ';')

		for _, path in pairs(string.split(paths, ';')) do
			for _, ext in pairs(exts) do
				local p = string.gsub(path, '*', ext)
				table.insert(pathtable, p)
			end
		end

		return table.concat(pathtable, ';')
	end

	haka._on_exit = {}

	function haka._exiting()
		for k, f in pairs(haka._on_exit) do
			haka._on_exit[k] = nil
			f()
		end
	end

	function haka.on_exit(func)
		table.insert(haka._on_exit, func)
	end

	package.cpath = addpath(package.cpath, haka.module_path(), { haka.module_prefix .. '?' .. haka.module_suffix })
	package.path = addpath(package.path, haka.module_path(), { '?.bc', '?.lua' })

	require('rule')
}
