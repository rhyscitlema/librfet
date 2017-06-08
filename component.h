#ifndef _COMPONENT_H
#define _COMPONENT_H
/*
    component.h
*/

#include "structures.h"

extern List* containers_list;

int pointer_compare (const void* key1, const void* key2, const void* arg);


/* NOTE: directly after call, set name=NULL and text=NULL */
Container* container_parse (Container* parent, lchar* name, lchar* text);

/* find and return a pointer to a container structure given its name */
Container *container_find (Container* current, const lchar* pathname, wchar* errormessage, bool skipFirst, bool fullAccess);

/* find and return a pointer to a component structure given its name */
Component *component_find (Container* current, const lchar* name, wchar* errormessage, bool skipFirst);

/* get and return the full path name of given container */
const wchar* container_path_name (const Container* container);


/* parse the string-expression of a component into an expression tree */
Component *component_parse (Component *component);

/* evaluate the expression tree of a component */
bool component_evaluate (ExprCallArg eca, Component *component,
                         const value* result_vst); // 'expected' result structure.


/* read: I 'component', am depending on 'depending' */
void depend_add (Component* component, Component *depending);

/* read: I 'depend', am not depended upon by 'component' */
void depend_denotify (List* depend, const Component *component);

bool dependence_parse ();
void dependence_finalise (bool success);


extern int evaluation_instance;
extern Container* replacement_container;

/* Record the replacement operator := */
void replacement_record (Container *c, const Expression *replacement);

/* Commit the replacement recorded */
void replacement_commit (Container *c);


#endif

