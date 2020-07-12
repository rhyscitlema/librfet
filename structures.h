#ifndef _STRUCTURES_H
#define _STRUCTURES_H
/*
    structures.h
*/

#include <_math.h>
#include <_string.h>
#include <_malloc.h>
#include <_texts.h>
#include <list.h>
#include <avl.h>

size_t stackSize();
value stackArray();

void rfet_init (size_t stack_size);
#define rfet_clean() rfet_init(0)


typedef struct _Expression
{
    // used for expression tree structure
    struct _Expression *parent;
    struct _Expression *headChild;
    struct _Expression *lastChild;
    struct _Expression *nextSibling;
    struct _Expression *prevSibling;

    // name of expression (name of operator or component or string)
    const_Str3 name;

    // used to store the ID of an operation.
    enum ID_TWSF ID;

    // type of the expression (AOPERATOR, ACONSTANT, AFUNCTION, ...)
    int type;

    // what the previous item type must be for valid syntax
    int previous;

    // precedence level, used when adding a new expression
    int precedence;

    // the component that owns this expression
    struct _Component *component;

    //union{

    // used for when expression is a call to a component
    struct _Component *call_comp;

    // used by expression_to_operation() for replacement
    const_value ptr_to_lhs;

    int param; // used to store the position of a parameter

    int outsider; // ID of outsider component call

    //};

    // = 0 => outsider dependent (default)
    // = 2 => parameter dependent (inside a function)
    // = 1 => independent, so can be made constant
    int info;

    // used by expression_destroy()
    bool deleted;
} Expression;


enum COMP_STATE {
    ISPARSE, // I have been parsed
    DOPARSE, // I have changed, parse me
    NOPARSE, // I have not changed, leave me alone
    ISFOUND, // I have been found in my container
    DOFOUND, // I have been found but was DOPARSE
    NOFOUND, // I have been placed on death sentence
    CREATED  // I have been created to serve YKW...
};

enum COMP_ACCESS {
    ACCESS_PRIVATE,     // in same container
    ACCESS_ENCLOSED,    // have same parents
    ACCESS_PROTECTED,   // have same grandpa
    ACCESS_PUBLIC,      // anyone anywhere
    ACCESS_REPLACE
};

static inline /*const*/ char* access2str (enum COMP_ACCESS access)
{
    switch(access) {
        case ACCESS_PRIVATE:   return "Private";
        case ACCESS_ENCLOSED:  return "Enclosed";
        case ACCESS_PROTECTED: return "Protected";
        case ACCESS_PUBLIC:    return "Public";
        case ACCESS_REPLACE:   return "Replace";
        default: return "NULL_ACCESS";
    }
}


typedef struct _Component Container;

typedef struct _Component
{
    Str3 name1;
    Str3 name2;

    Str3 text1; // expression of main component,
    Str3 text2; // to be parsed to produce oper2

    enum COMP_ACCESS access1; // access control type
    enum COMP_ACCESS access2; // see enum COMP_ACCESS above

    int replace; // count of ACCESS_REPLACE overrides

    enum COMP_STATE state; // see enum COMP_STATE above

    Container *parent; // containing container


    // used when component is a function:
    uint32_t para1[200]; // the function parameter.
    uint32_t para2[200]; // para[0] = 1 + number of paras.

    // if para[0]==1 then function has 0 parameter.
    // if para[0]==0 then component is a variable.
    // function can have at most 255 parameters.

    // used when component is a variable:
    value constant;     // obtained during evaluation.
    long  instance;     // the evaluation instance.
    Container *caller;  // the used calling container.


    // operations-array obtained from parsing text2:
    value oper1; // this is what gets evaluated
    value oper2; // must assert(oper && *oper)

    // the expected result value structure
    uint32_t expc1[200];
    uint32_t expc2[200];

    // used by the ':=' operator during evaluation
    AVLT replacement;

    AVLT depOnMe; // components that depend on me
    AVLT depend1; // components depended upon by me
    AVLT depend2;


    // used when component is a container:

    Str3 rfet1;         // Rhyscitlem Function
    Str3 rfet2;         // Expression Text (RFET)

    Container *type1;   // the inherited container
    Container *type2;   // from: type = "<name>";

    AVLT inherits;      // inheriting components
    AVLT inners;        // inner components

    void* owner;        // used by an external program
} Component;

#define c_type(c)       ( (c)->name2.ptr ? (c)->type2      : (c)->type1  )
#define c_name(c)       ( (c)->name2.ptr ? (c)->name2      : (c)->name1  )
#define c_text(c)       ( (c)->name2.ptr ? (c)->text2      : (c)->text1  )
#define c_para(c)       ( (c)->name2.ptr ? (c)->para2      : (c)->para1  )
#define c_oper(c)       ( (c)->name2.ptr ? (c)->oper2      : (c)->oper1  )
#define c_expc(c)       ( (c)->name2.ptr ? (c)->expc2      : (c)->expc1  )
#define c_rfet(c)       ( (c)->name2.ptr ? (c)->rfet2      : (c)->rfet1  )
#define c_access(c)     ( (c)->name2.ptr ? (c)->access2    : (c)->access1)
#define c_depend(c)     ( (c)->name2.ptr ? (c)->depend2    : (c)->depend1)
#define c_iscont(c)     ( c_rfet(c).ptr ) // = false for Root Container
#define c_container(c)  ( c_iscont(c) ? (c) : (c)->parent )


void component_destroy (Component *component);

Str2 inherits_obtain (Component *component, List* list, Str2 out, const char* text, int indent);
bool inherits_remove (Component *component);

void component_print (const char* text, int indent, const Component *component);
// 'indent' can be -2, -1 or >=0, these 3 have different behaviours

bool CheckComponent (Component* component, bool finalised);
bool CheckStr3 (const_Str3 str); // check for pointer errors

#endif
