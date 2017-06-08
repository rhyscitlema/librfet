#ifndef _OUTSIDER_H
#define _OUTSIDER_H
/*
    outsider.h

    See outsider_default.c
*/

#include <structures.h>

#define OUTSIDER_ISA_FUNCTION 0x10000000
#define GET_OUTSIDER_ID(expression) (expression)->param[0]
#define SET_OUTSIDER_ID(expression,ID) (expression)->param[0]=ID;

extern bool user_input_allowed;

/* Return outsider's ID > 0, else return 0 */
unsigned int outsider_getID (const lchar* lstr);

/* Evaluate outsider expression */
bool set_outsider (ExprCallArg eca);

/* Called by component_parse() */
bool checkfail (const Container* c, const lchar* name);

#endif

