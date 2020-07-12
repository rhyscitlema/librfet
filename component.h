#ifndef _COMPONENT_H
#define _COMPONENT_H
/*
	component.h
*/

#include "structures.h"

extern List* container_list();

/* item1 and item2 are void**, see the implementation inside component.c */
int pointer_compare (const ITEM* item1, const ITEM* item2, const void* arg);


/* NOTE: directly after call always set name=NULL and text=NULL */
Container* container_parse (value stack, Container* parent, Str3 name, Str3 text);

/* find and return a Container* given its pathname, may disregard access control */
Container *container_find (value stack, Container* current, const_Str3 pathname, bool fullAccess);

/* find and return a Component* given its name, may skip first one found */
Component *component_find (value stack, Container* current, const_Str3 name, bool skipFirst);

/* build and return the full path+name of given container */
Str2 container_path_name (value stack, const Container* container);


/* check if name has '|' or is "." or ".." */
bool isSpecial3 (const_Str3 name);

/* parse the expression-string of given component into an operations-array */
Component* component_parse (value stack, Component *component);

/* evaluate the operations-array of given component */
value component_evaluate (
	value stack,
	Container *caller,
	Component *component,
	const_value argument );


/* read: I <component> am depending on <depending> */
void depend_add (Component* component, Component *depending);

/* read: I <depend> am not depended upon by <component> */
void depend_denotify (Tree* depend, const Component *component);

bool dependence_parse (value stack);
void dependence_finalise (bool success);


/* used mainly by replacement operator */
long evaluation_instance (bool increment);

/* Record the replacement operator upon a LHS change */
void replacement_record (const_value repl);

enum REPL_OPERATION {
	REPL_CANCEL,    // Cancel the replacement recorded
	REPL_COMMIT,    // Commit the replacement recorded
	REPL_COUNT,     // Count the replacement recorded
	// note: <stack> only needed if on REPL_COMMIT
};
long replacement (value stack, Container *c, enum REPL_OPERATION opr);


#endif

