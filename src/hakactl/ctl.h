
#ifndef _CTL_H
#define _CTL_H

#include <haka/types.h>


int  ctl_open_socket();
bool ctl_send(int fd, const char *command);
bool ctl_print(int fd);
bool ctl_check(int fd, const char *str);

#endif /* _CTL_H */
