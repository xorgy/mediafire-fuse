#ifndef _STR_TOOLS_H_
#define _STR_TOOLS_H_

#include <stdint.h>

char*       strdup_printf(char* fmt, ...);

char*       strdup_join(char *string1,char *string2);

char*       strdup_subst(char *string,char *str_old,char *str_new,int max);

void        string_chomp(char *string);

// void        string_strip_head(char *string,char c);

// void        string_strip_tail(char *string,char c);

#endif
