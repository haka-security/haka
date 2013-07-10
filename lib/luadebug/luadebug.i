%module luadebug

%{
	#include <luadebug/debugger.h>
	#include "interactive.h"

	#define new_luadebug_debugger()      luadebug_debugger_create(L)
	#define new_luadebug_interactive()   luadebug_interactive_create(L)
%}

%rename(interactive) luadebug_interactive;
struct luadebug_interactive {
	%extend {
		luadebug_interactive();

		~luadebug_interactive() {
			luadebug_interactive_cleanup($self);
		}

		void start() {
			luadebug_interactive_enter($self);
		}

		void setprompt(const char *single, const char *multi) {
			luadebug_interactive_setprompts($self, single, multi);
		}
	}
};

%rename(debugger) luadebug_debugger;
struct luadebug_debugger {
	%extend {
		luadebug_debugger();

		~luadebug_debugger() {
			luadebug_debugger_cleanup($self);
		}

		void stop() {
			luadebug_debugger_break($self);
		}
	}
};

%luacode {
	local color = require("color")

	function luadebug.hide_underscore(name)
		if type(name) == "string" then
			return name:sub(1, 1) == "_"
		else
			return false
		end
	end

	local function __pprint(obj, indent, name, visited, hidden, depth)
		local type = type(obj)

		local title
		if name then
			title = table.concat({indent, color.blue, color.bold, name, color.clear, " : "})
		else
			title = ""
		end

		if type == "userdata" or type == "table" then
			if visited[obj] then
				print(table.concat({title, color.red, "recursive value", color.clear}))
				return
			end

			visited[obj] = true
		end

		if type == "table" then
			title = table.concat({title, color.cyan, color.bold, "table", color.clear})
			if depth == 0 then
				print(title)
			else
				print(title, "{")

				for key, value in pairs(obj) do
					if not hidden or not hidden(key) then
						__pprint(value, indent .. "  ", key, visited, hidden, depth-1)
					end
				end

				print(indent .. "}")
			end
		elseif type == "userdata" then
			local meta = getmetatable(obj)
			if meta then
				title = table.concat({title, color.cyan, color.bold, "userdata", color.clear})

				if meta[".type"] then
					title = table.concat({title, " ", tostring(meta[".type"])})
				end

				if meta[".fn"] and meta[".fn"].__tostring then
					print(title, tostring(obj))
				else
					if depth == 0 then
						print(title)
					else
						local has_value = false

						for key, _ in pairs(meta[".get"]) do
							if not hidden or not hidden(key) then
								if not has_value then
									print(title, "{")
									has_value = true
								end

								local success, child_obj = pcall(function () return obj[key] end)
								if success then
									__pprint(child_obj, indent .. "  ", key, visited, hidden, depth-1)
								else
									local error = table.concat({indent, "  ", color.blue, color.bold, key, color.clear, " : ", color.red, child_obj, color.clear})
									print(error)
								end
							end
						end

						if has_value then
							print(indent .. "}")
						else
							print(title)
						end
					end
				end
			else
				print(table.concat({title, color.cyan, color.bold, "userdata", color.clear}))
			end
		elseif type == "function" then
			print(table.concat({title, color.cyan, color.bold, "function", color.clear}))
		elseif type == "string" then
			print(table.concat({title, color.magenta, color.bold, "\"", obj, "\"", color.clear}))
		elseif type == "boolean" then
			print(table.concat({title, color.magenta, color.bold, tostring(obj), color.clear}))
		elseif type == "thread" then
			print(table.concat({title, color.cyan, color.bold, "thread", color.clear}))
		else
			print(title .. tostring(obj))
		end

		if type == "userdata" or type == "table" then
			visited[obj] = nil
		end
	end

	function luadebug.pprint(obj, indent, depth, hide)
		if num then
			__pprint(obj, indent or "", nil, {}, hide, depth or -1)
		else
			__pprint(obj, indent or "", nil, {}, hide, depth or -1)
		end
	end
}
