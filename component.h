#ifndef _COMPONENT_H
#define _COMPONENT_H
/*
    component.h
*/

#include "structures.h"


/* Create a new container and add to the linked-list of containers. */
/* Parse the container content into a linked-list of components. */
bool container_add (Container** container_ptr, const lchar* name, const lchar* content);


/* check for and return container if it exists */
bool container_check (const Container* container);

/* find and return a pointer to a container structure given its name */
Container *container_find (const lchar* name);

/* find and return a pointer to a component structure given its name */
Component *component_find (Container* container, const lchar* name);


/* parse the string-expression of a component into an expression tree */
Component *component_parse (Component *component, Container *container);

/* evaluate the expression tree of a component, given its function arguments */
Value* component_evaluate (Component *component,
                           const Value* argument,   // in case of a function call
                           const Value* result_vst);// 'expected' result structure


Depend* depend_find (Depend* depend, const Component *component);

/* read: I 'component', am depending on 'depending' */
void depend_add (Component* component, Component *depending);

/* read: I 'depend', am not depended upon by 'component' */
void depend_denotify (Depend* depend, const Component *component);


bool dependence_parse ();
bool dependence_evaluate ();
void dependence_finalise (bool success);


extern int evaluation_instance;
extern Container* replacement_container;

/* Record the replacement operator := */
void replacement_record (Container *c, const Expression *replace);

/* Commit the replacement recorded */
void replacement_commit (Container *c);


// TODO: remove this getmfetc from here!
inline static lchar* getmfetc (Container* container, const char* comp_name)
{
    lchar* name = NULL;
    Expression* expr = NULL;
    Component *comp = component_find(container, CST31(comp_name));
    if(comp) expr = (comp->root2==NULL) ? comp->root1 : comp->root2;
    if(expr) astrcpy33(&name, expr->name);
    return name;
}

#endif

