#ifndef _STRUCTURES_H
#define _STRUCTURES_H
/*
    structures.h

    Component > Container | Function > Expression > value
*/

#include <_math.h>
#include <_string.h>
#include <_malloc.h>
#include <_texts.h>
#include <assert.h>
#include <list.h>
#include <avl.h>

void rfet_init (int stack_size);
void rfet_clean ();

int stackSize();
value* mainStack();


typedef struct _GeneralArg
{   struct _Component* caller;
    wchar* message;
    const value* argument;
} GeneralArg;

typedef struct _ExprCallArg
{   GeneralArg* garg;
    struct _Expression* expression;
    value* stack;
} ExprCallArg;



typedef struct _Expression
{
    // used for expression tree structure
    struct _Expression *parent;
    struct _Expression *headChild;
    struct _Expression *lastChild;
    struct _Expression *nextSibling;
    struct _Expression *prevSibling;

    // name of expression (name of operator or component or string)
    lchar* name;

    // used to store the ID of an operation.
    enum ID_TWSF ID;

    // type of the expression (AOPERATOR, ACONSTANT, AFUNCTION, ...)
    int type;

    // what the previous item type must be for valid syntax
    int previous;

    // precedence level, used when adding a new expression
    int precedence;

    bool (*evaluate) (ExprCallArg eca);

    // the component that owns this expression
    struct _Component *component;

    // used for when expression is a call to a component
    struct _Component *call_comp;

    // used to store the position of a parameter.
    // used as outsider ID for when expression is an outsider.
    int param[20];

    // used for when expression evaluates to a constant
    value constant;

    // used to store first-time evaluation information
    int info;

    // = 0 => outsider dependent (default)
    // = 2 => parameter dependent (inside a function)
    // = 1 => independent
    char independent;

    // used by expression_destroy()
    bool deleted;
} Expression;

#define expression_evaluate(eca) (eca.expression!=NULL ? eca.expression->evaluate(eca) : false)



enum STATE {
    ISPARSE, // I have been parsed
    DOPARSE, // I have changed, parse me
    NOPARSE, // I have not changed, do not parse me
    DELETED, // I have not been found in my container
    CREATED  // I have just been created
};

enum ACCESS {
    ACCESS_PRIVATE,     // in same container
    ACCESS_ENCLOSED,    // have same parents
    ACCESS_PROTECTED,   // have same grandpa
    ACCESS_PUBLIC,      // anyone anywhere
    ACCESS_REPLACE
};

static inline const char* access2str (enum ACCESS access)
{
    switch(access) {
        case ACCESS_PRIVATE:   return "Private";
        case ACCESS_ENCLOSED:  return "Enclosed";
        case ACCESS_PROTECTED: return "Protected";
        case ACCESS_PUBLIC:    return "Public";
        case ACCESS_REPLACE:   return "Replace";
        default: return "NULL";
    }
}



// Note: it is not possible to create separate
// structures: Function and Container, both
// inheriting from Component, just because
// of the possibility of one converting to
// the other in the process of parsing.
typedef struct _Component Container;

typedef struct _Component
{
    lchar* name1;
    lchar* name2;

    lchar* text1;
    lchar* text2;

    bool isaf1; // is a function or
    bool isaf2; // else a container

    enum ACCESS access1;
    enum ACCESS access2;

    enum STATE state;

    int replace; // count of replace-access overrides

    Container *parent;


    // Component as a Function
    Expression* root1;
    Expression* root2;

    value* para1; // parameter
    value* para2;

    List depOnMe;
    List depend1;
    List depend2;

    value* result1;
    value* result2;


    // Component as a Container
    struct _Component *type1;
    struct _Component *type2;

    lchar* rfet1;
    lchar* rfet2;
    void* owner;

    AVLT inherits;  // inheriting components
    AVLT inners;    // inner components

    int replacement_count;
    const Expression* replacement[20];

} Component;

#define c_type(c)       ((c)->type2==NULL ? (c)->type1      : (c)->type2  )
#define c_name(c)       ((c)->name2==NULL ? (c)->name1      : (c)->name2  )
#define c_text(c)       ((c)->name2==NULL ? (c)->text1      : (c)->text2  )
#define c_para(c)       ((c)->name2==NULL ? (c)->para1      : (c)->para2  )
#define c_isaf(c)       ((c)->name2==NULL ? (c)->isaf1      : (c)->isaf2  )
#define c_root(c)       ((c)->name2==NULL ? (c)->root1      : (c)->root2  )
#define c_rfet(c)       ((c)->name2==NULL ? (c)->rfet1      : (c)->rfet2  )
#define c_access(c)     ((c)->name2==NULL ? (c)->access1    : (c)->access2)
#define c_result(c)     ((c)->name2==NULL ? (c)->result1    : (c)->result2)
#define c_depend(c)     ((c)->name2==NULL ? (c)->depend1    : (c)->depend2)
#define c_container(c)  (c_isaf(c) ? (c)->parent : (c))



void expression_remove (Expression *expression);
value* expression_to_valueSt (Expression *expression);

void component_destroy (Component *component);
void component_remove (AVLT* tree);

void inherits_obtain (Component *component, List* list, wchar* mstr, const char* text, int indent);
bool inherits_remove (Component *component);

void expression_print (const char* text, int indent, const Expression *expression);
void component_print  (const char* text, int indent, const Component  *component );

#endif

