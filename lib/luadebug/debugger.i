
%{
	#include "debugger.h"

	#define new_luadebug_debugger()   luadebug_debugger_create(L)
%}

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
