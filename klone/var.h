#ifndef _KLONE_VAR_H_
#define _KLONE_VAR_H_
#include <sys/types.h>
#include <klone/str.h>

struct var_s;
typedef struct var_s var_t;

int var_create(const char* name, const char *value, var_t**);
int var_free(var_t*);

const char* var_get_name(var_t *v);
const char* var_get_value(var_t *v);

string_t* var_get_name_s(var_t *v);
string_t* var_get_value_s(var_t *v);

int var_set(var_t*, const char *name, const char *value);
int var_set_name(var_t *v, const char *name);
int var_set_value(var_t *v, const char *value);

#endif
