
#ifndef _STATE_H
#define _STATE_H

#include <wchar.h>

typedef struct lua_State lua_state;

lua_state *init_state();
void cleanup_state(lua_state *L);
void print_error(lua_state *L, const wchar_t *msg);
int run_file(lua_state *L, const char *filename, int argc, char *argv[]);
int do_file_as_function(lua_state *L, const char *filename);
int lua_error_formater(lua_state *L);

#endif /* _STATE_H */

