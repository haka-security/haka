
#ifndef _HAKA_PARAMETERS_H
#define _HAKA_PARAMETERS_H

#include <stddef.h>
#include <haka/types.h>


/* Opaque structures. */
struct parameters;

struct parameters *parameters_open(const char *file);
struct parameters *parameters_create();
void               parameters_free(struct parameters *params);

int                parameters_open_section(struct parameters *params, const char *section);
int                parameters_close_section(struct parameters *params);

const char        *parameters_get_string(struct parameters *params, const char *key, const char *def);
bool               parameters_get_boolean(struct parameters *params, const char *key, bool def);
int                parameters_get_integer(struct parameters *params, const char *key, int def);

bool               parameters_set_string(struct parameters *params, const char *key, const char *value);
bool               parameters_set_boolean(struct parameters *params, const char *key, bool value);
bool               parameters_set_integer(struct parameters *params, const char *key, int value);


#endif /* _HAKA_PARAMETERS_H */
