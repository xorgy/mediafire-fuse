#ifndef _MFSHELL_JSON_H_
#define _MFSHELL_JSON_H_

#include <jansson.h>

json_t* json_object_by_path(json_t *start,const char *path);

#endif


