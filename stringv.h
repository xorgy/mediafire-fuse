#ifndef _STRING_V_H_
#define _STRING_V_H_

#include <stdint.h>

#define     STRINGV_FREE_ALL    1

// count number of strings in a NULL in a string vector
size_t      stringv_len(char **array);

// free all of the strings in a vector and optionally the vector pointer
void        stringv_free(char **array,int b_free);

// deep copy of string vector.  returns a new vector pointer
char**      stringv_copy(char **array);

// returns a NULL terminated vector array to every location 'token' is found
char**      stringv_find(char *string,char *token,int limit);

// returns a NULL terminated vector array of items delimited by 'token'
char**      stringv_split(char *string,char *token,int limit);



#endif
