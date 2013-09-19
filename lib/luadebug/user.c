
#include <luadebug/user.h>

#include <stdio.h>


void luadebug_user_init(struct luadebug_user *user)
{
	atomic_set(&user->refcount, 1);
}

void luadebug_user_addref(struct luadebug_user *user)
{
	atomic_inc(&user->refcount);
}

void luadebug_user_release(struct luadebug_user **user)
{
	if (*user) {
		if (atomic_dec(&(*user)->refcount) == 0) {
			(*user)->destroy(*user);
		}
		*user = NULL;
	}
}
