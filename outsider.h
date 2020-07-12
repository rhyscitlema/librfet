#ifndef _OUTSIDER_H
#define _OUTSIDER_H
/*
	outsider.h

	See outsider_default.c
*/

#include <structures.h>

#define OUTSIDER_ISA_FUNCTION 0x10000000

extern bool user_input_allowed;

/* Return outsider's ID != 0, else return 0 */
int outsider_getID (const_Str3 str);

/* Evaluate outsider expression */
value set_outsider (value stack, int ID);


/* Called by container_parse() in component.c.
*  If name!=NULL then c = parent container.
*  See librodt/tools.c
*/
bool checkfail (value stack, const Container* c, const_Str3 name, bool hasType, bool isNew);

#endif

