%module luadebug

%{
	#include <luadebug/debugger.h>
	#include "debugger.h"
	#include <luadebug/interactive.h>

	#define lua_luadebug_interactive_enter(single, multi, msg) luadebug_interactive_enter(L, single, multi, msg)

	#define lua_luadebug_debugger_start(break_immediatly) luadebug_debugger_start(L, break_immediatly)
	#define lua_luadebug_debugger_stop()                  luadebug_debugger_stop(L)
	#define lua_luadebug_debugger_break()                 luadebug_debugger_break(L)


	void lua_pushpdebugger(lua_State *L, struct luadebug_debugger *dbg)
	{
		SWIG_NewPointerObj(L,dbg,SWIGTYPE_p_luadebug_debugger,1);
	}

	struct luadebug_debugger *lua_getpdebugger(lua_State *L, int index)
	{
		struct luadebug_debugger *dbg = NULL;
		if (!SWIG_IsOK(SWIG_ConvertPtr(L, index, (void**)&dbg, SWIGTYPE_p_luadebug_debugger, 0))) {
			return NULL;
		}
		return dbg;
	}
%}

%include "haka/lua/swig.si";

%nodefaultctor;
%nodefaultdtor;

%rename(__interactive_enter) lua_luadebug_interactive_enter;
void lua_luadebug_interactive_enter(const char *single, const char *multi, const char *msg);

%rename(__debugger) luadebug_debugger;
struct luadebug_debugger {
	%extend {
		luadebug_debugger() {
			return NULL;
		}

		~luadebug_debugger() {
			luadebug_debugger_cleanup($self);
		}
	}
};

%rename(__debugger_start) lua_luadebug_debugger_start;
void lua_luadebug_debugger_start(bool break_immediatly=false);

%rename(__debugger_stop) lua_luadebug_debugger_stop;
void lua_luadebug_debugger_stop();

%rename(__debugger_break) lua_luadebug_debugger_break;
void lua_luadebug_debugger_break();

%luacode {
	local this = unpack({...})
	local color = require("color")

	function this.hide_underscore(name)
		if type(name) == "string" then
			return name:sub(1, 1) == "_"
		else
			return false
		end
	end

	function this.__printwrapper(p, data)
		return function (...)
			p(data, ...)
		end
	end

	local function __pprint(obj, indent, name, visited, hidden, depth, out)
		local type = type(obj)

		local title
		if name then
			title = table.concat({indent, color.blue, color.bold, name, color.clear, " : "})
		else
			title = ""
		end

		if type == "userdata" or type == "table" then
			if visited[obj] then
				out(table.concat({title, color.red, "recursive value", color.clear}))
				return
			end

			visited[obj] = true
		end

		if type == "table" then
			title = table.concat({title, color.cyan, color.bold, "table", color.clear})
			if depth == 0 then
				out(title)
			else
				out(title, "{")

				for key, value in pairs(obj) do
					if not hidden or not hidden(key) then
						__pprint(value, indent .. "  ", tostring(key), visited, hidden, depth-1, out)
					end
				end

				out(indent .. "}")
			end
		elseif type == "userdata" then
			local meta = getmetatable(obj)
			if meta and meta[".type"] and meta[".get"]then
				title = table.concat({title, color.cyan, color.bold, "userdata", color.clear})

				if meta[".type"] then
					title = table.concat({title, " ", tostring(meta[".type"])})
				end

				if meta[".fn"] and meta[".fn"].__tostring then
					out(title, tostring(obj))
				else
					if depth == 0 then
						out(title)
					else
						local has_value = false

						for key, _ in pairs(meta[".get"]) do
							if not hidden or not hidden(key) then
								if not has_value then
									out(title, "{")
									has_value = true
								end

								local success, child_obj = pcall(function () return obj[key] end)
								if success then
									__pprint(child_obj, indent .. "  ", key, visited, hidden, depth-1, out)
								else
									local error = table.concat({indent, "  ", color.blue, color.bold, key, color.clear, " : ", color.red, child_obj, color.clear})
									out(error)
								end
							end
						end

						if has_value then
							out(indent .. "}")
						else
							out(title)
						end
					end
				end
			else
				out(table.concat({title, color.cyan, color.bold, "userdata", color.clear}))
			end
		elseif type == "function" then
			out(table.concat({title, color.cyan, color.bold, "function", color.clear}))
		elseif type == "string" then
			out(table.concat({title, color.magenta, color.bold, "\"", obj, "\"", color.clear}))
		elseif type == "boolean" then
			out(table.concat({title, color.magenta, color.bold, tostring(obj), color.clear}))
		elseif type == "thread" then
			out(table.concat({title, color.cyan, color.bold, "thread", color.clear}))
		else
			out(title .. tostring(obj))
		end

		if type == "userdata" or type == "table" then
			visited[obj] = nil
		end
	end

	function this.pprint(obj, indent, depth, hide, out)
		if num then
			__pprint(obj, indent or "", nil, {}, hide, depth or -1, out or print)
		else
			__pprint(obj, indent or "", nil, {}, hide, depth or -1, out or print)
		end
	end

	this.debugger = {}
	this.debugger.start = this.__debugger_start
	this.debugger.stop = this.__debugger_stop
	this.debugger.breakpoint = this.__debugger_break

	this.interactive = {}
	this.interactive.enter = this.__interactive_enter
}
