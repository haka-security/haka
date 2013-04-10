
#ifndef _IPTABLES_H
#define _IPTABLES_H

int apply_iptables(const char *conf);
int save_iptables(const char *table, char **conf);

#endif /* _IPTABLES_H */
