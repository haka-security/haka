/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module haka

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

%}

%include "haka/lua/swig.si"

%include "lua/object.si"

%nodefaultctor;
%nodefaultdtor;

%rename(current_thread) thread_getid;
int thread_getid();

%rename(exit) haka_exit;
void haka_exit();

struct time {
	int    secs;
	int    nsecs;

	%extend {
		time(double ts) {
			struct time *t = malloc(sizeof(struct time));
			if (!t) {
				error("memory error");
				return NULL;
			}

			time_build(t, ts);
			return t;
		}

		~time() {
			free($self);
		}

		%immutable;
		double seconds { return time_sec($self); }

		void __tostring(char **TEMP_OUTPUT)
		{
			*TEMP_OUTPUT = malloc(TIME_BUFSIZE);
			if (!*TEMP_OUTPUT) {
				error("memory error");
				return;
			}

			if (!time_tostring($self, *TEMP_OUTPUT, TIME_BUFSIZE)) {
				assert(check_error());
				free(*TEMP_OUTPUT);
				*TEMP_OUTPUT = NULL;
			}
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(time);

%native(_threads_info) int threads_info(lua_State *L);

%{
	int threads_info(struct lua_State *L)
	{
		int i;

		lua_newtable(L);

		for (i=0;; ++i) {
			struct engine_thread *engine = engine_thread_byid(i);
			if (!engine) break;

			volatile struct packet_stats *packet_stats = engine_thread_statistics(engine);

			lua_pushnumber(L, i+1);

			lua_newtable(L);

			lua_pushnumber(L, i);
			lua_setfield(L, -2, "id");

			switch (engine_thread_status(engine)) {
			case THREAD_RUNNING: lua_pushstring(L, "running"); break;
			case THREAD_WAITING: lua_pushstring(L, "waiting"); break;
			case THREAD_DEBUG:   lua_pushstring(L, "debug"); break;
			case THREAD_DEFUNC:  lua_pushstring(L, "defunc"); break;
			case THREAD_STOPPED: lua_pushstring(L, "stopped"); break;
			default:             assert(0);
			}

			lua_setfield(L, -2, "status");

			lua_pushnumber(L, (double)packet_stats->recv_packets);
			lua_setfield(L, -2, "recv_pkt");
			lua_pushnumber(L, (double)packet_stats->recv_bytes);
			lua_setfield(L, -2, "recv_bytes");
			lua_pushnumber(L, (double)packet_stats->trans_packets);
			lua_setfield(L, -2, "trans_pkt");
			lua_pushnumber(L, (double)packet_stats->trans_bytes);
			lua_setfield(L, -2, "trans_bytes");
			lua_pushnumber(L, (double)packet_stats->drop_packets);
			lua_setfield(L, -2, "drop_pkt");

			lua_settable(L, -3);
		}

		return 1;
	}
%}

%luacode {
	haka = unpack({...})

	require('class')
	require('utils')
	require('events')

	haka.events = {}

	haka.events.exiting = haka.event.Event:new("exiting")
	haka.events.started = haka.event.Event:new("started")

	haka.helper = {}

	function haka.abort()
		error(nil)
	end

	haka.console = {}

	haka.console.threads = haka._threads_info
	haka._threads_info = nil
}

%include "lua/vbuffer.si"
%include "lua/regexp.si"
%include "lua/packet.si"
%include "lua/alert.si"
%include "lua/log.si"
%include "lua/state_machine.si"

%luacode {
	require('context')
	require('dissector')
	require('grammar')

	haka.mode = 'normal'
}
