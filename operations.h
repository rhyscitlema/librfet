#ifndef _OPERATIONS_H
#define _OPERATIONS_H
/*
    operations.h
*/
#include <expression.h>
#include <outsider.h>

void operations_init();


#ifndef IGNORE
#define IGNORE          0x000
#endif

#define SkipClimbUp     0x001
#define RightAssoct     0x002
#define OPENBRACKET     0x004
#define CLOSEBRACKET    0x008

#define ACONSTANT       0x010
#define AVARIABLE       0x020
#define AFUNCTION       0x040
#define APARAMETER      0x080

#define ACOMMA          0x100
#define ABASIC          0x200
#define ACOMPARE        0x400
#define ALOGICAL        0x800
#define AOPERATOR       (ACOMMA | ABASIC | ACOMPARE | ALOGICAL)

#define ALEAVE          (CLOSEBRACKET | ACONSTANT | AVARIABLE | APARAMETER)
#define NOTLEAVE        (OPENBRACKET | AOPERATOR)

#define HIGHEST 50


/* the below are used in expression.c */
extern Expression character_type[80];  // character_type means it can be directly followed by any type (ex: +)
extern Expression opr_word_type[10];   // opr_word_type means loaded as a word_type although is an operator (ex: mod)
extern Expression word_type[60];       // word_type means it must be inbetween space_type or character_type (ex: pi)

extern Expression number_type;          // is a number like 1, 23
extern Expression string_type;          // is a string, like "file"
extern Expression variable_type;        // is a user defined variable
extern Expression function_type;        // is a user defined function
extern Expression parameter_type;       // is a parameter to a user defined function
extern Expression outsider_type;        // is a variable with name defined from outside
extern Expression current_type;         // is a variable used by the replacement operation
extern Expression contcall_type;        // is a '.' which is used for container call
extern Expression indexing_type;


//-------------------------------------------------------------------------------

#define NOT_YET { set_message(eca.garg->message, L"Error in \\1 at \\2:\\3:\r\nOn '\\4': operation not yet available!", eca.expression->name); return 0; }

#define MIND(A,B) ((A==0 || B==0) ? 0 : (A==2 || B==2) ? 2 : 1) // Mix Independence

static inline value toSingle (const value* stack)
{
    value v;
    if(VST_LEN(stack)==1) v = *stack;
    else v = setPoiter(value_copy(stack));
    return v;
}


#endif /* _OPERATIONS_H */
