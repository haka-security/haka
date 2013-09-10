
#ifndef _STATE_H
#define _STATE_H

#include <wchar.h>
#include <haka/lua/state.h>

struct lua_State;

struct lua_state *haka_init_state();
int run_file(struct lua_State *L, const char *filename, int argc, char *argv[]);
int do_file_as_function(struct lua_State *L, const char *filename);

#endif /* _STATE_H */

