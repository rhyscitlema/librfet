#ifndef _STRUCTURES_H
#define _STRUCTURES_H
/*
    structures.h

    Container > Component > Expression > Value
*/

#include <_string.h>
#include <_malloc.h>
#include <_math.h>



typedef struct _Expression
{
    // name of expression (name of operator or function or variable ...)
    lchar* name;

    // used as outsider ID for when expression is an outsider.
    // also used to store the TWSF-ID of an operation.
    int ID;

    // type of the expression (AOPERATOR, ACONSTANT, AVARIABLE, ...)
    int type;

    // what the previous item type must be for valid syntax
    int previous;

    // precedence level, used when adding a new expression
    int precedence;

    // function to call so to evaluate this expression
    Value* (*evaluate) (struct _Expression *expression, const Value* argument);

    // used only for when expression is a call to a component
    // that is, a call to a user-defined variable or function
    struct _Component *component;

    // used to store the position of a parameter.
    int param[20];

    // used for when expression evaluates to a constant
    Value* constant;

    // used to store first-time evaluation information
    int info;

    // = 0 => outsider dependent (default)
    // = 2 => parameter dependent (inside a function)
    // = 1 => independent
    char independent;

    // used only in expression_destroy() to mark as deleted
    bool deleted;

    // used for expression tree structure
    struct _Expression *parent;
    struct _Expression *nextChild;
    struct _Expression *prevChild;
    struct _Expression *headChild;
    struct _Expression *lastChild;
    struct _Expression *next;
} Expression;

#define expression_evaluate(expr,argument) ((expr==NULL) ? NULL : expr->evaluate(expr,argument))



typedef struct _Depend
{
    struct _Component* component;
    struct _Depend *prev, *next;
} Depend;



typedef struct _Component
{
    lchar* name1;
    lchar* name2;

    lchar* strExpr1;
    lchar* strExpr2;

    Value* cname2;

    Value* parameter1;
    Value* parameter2;

    Depend* dependOnMe;
    Depend* depend1;
    Depend* depend2;

    Expression* root1;
    Expression* root2;

    bool isprivate1;
    bool isprivate2;

    int state;

    struct _Container *container;
    struct _Component *nextToParse;
    struct _Component *prev, *next;
} Component;



typedef struct _Container
{
    lchar* name;
    lchar* type;
    lchar* text;
    Value* result;
    void* parent; // whoever owns this container

    int quickedit;
    int replacement_count;
    bool replacement_exist;
    const Expression* replace[20];

    struct _Component *first, *mainc;
    struct _Container *prev, *next;
} Container;



extern Expression *expression_new (const Expression *newExpr);
extern void expression_remove (Expression *expression);
extern Value* expression_to_valueSt (Expression *expression);


extern Depend *depend_new (Component *component);
extern void depend_destroy (Depend *depend);
extern void depend_remove (Depend* depend_head);

extern void depend_print (const Depend* depend_head);
extern int  depnd_count (const Depend* depend_head);


extern Component *component_new ();
extern void component_destroy (Component *component);
extern void component_remove (Component* component_head);
#define GCN(c) ((c)->name1 ? (c)->name1 : (c)->name2)


extern Container *container_new ();
extern void container_destroy (Container *container);
extern void container_remove (Container *container);


extern void expression_print (const char* text, const Expression *expression);
extern void component_print  (const char* text, const Component  *component );
extern void container_print  (const char* text, const Container  *container );


mchar* dependency_get_traverse (const char* text,   // can be NULL
                                const int spaces,   // typical is 4
                                const Component *component,
                                mchar* output);     // can be NULL

mchar* valueSt_get_traverse (const int spaces, const Value* valueSt, mchar* output);


extern void structures_print_count ();

#endif

