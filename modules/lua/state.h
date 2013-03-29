
#ifndef _STATE_H
#define _STATE_H

struct lua_State *init_state();
void cleanup_state(struct lua_State *state);

#endif /* _STATE_H */

