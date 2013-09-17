
#ifndef _CTL_H
#define _CTL_H

#include <haka/types.h>


int  ctl_open_socket();
bool ctl_send(int fd, const char *command);
bool ctl_receive_chars(int fd, char *buffer, size_t len);
bool ctl_receive_wchars(int fd, wchar_t *buffer, size_t len);
bool ctl_check(int fd, const char *str);

#endif /* _CTL_H */
