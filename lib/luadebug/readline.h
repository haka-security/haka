
#ifndef _LUAINTERACT_READLINE_H
#define _LUAINTERACT_READLINE_H

#include <editline/readline.h>

void init_readline(const char *name, CPPFunction* complete);

#endif /* _LUAINTERACT_READLINE_H */
