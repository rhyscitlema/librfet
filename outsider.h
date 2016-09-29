#ifndef _OUTSIDER_H
#define _OUTSIDER_H
/*
    outsider.h
*/

#include <structures.h>

extern bool user_input_allowed;

/* Return outsider's ID > 0, else return 0.
   Time 't' is also considered an outsider! */
unsigned int outsider_getID (const lchar* lstr);

Value* set_outsider (Expression* expression, const Value* argument);

#endif

