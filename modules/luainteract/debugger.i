
%{
	#include "debugger.h"

	#define new_luainteract_debugger()   luainteract_debugger_create(L)
%}

%rename(debugger) luainteract_debugger;

struct luainteract_debugger {
	%extend {
		luainteract_debugger();

		~luainteract_debugger() {
			luainteract_debugger_cleanup($self);
		}

		void stop() {
			luainteract_debugger_break($self);
		}
	}
};
