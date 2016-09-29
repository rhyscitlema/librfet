#ifndef _OPERATIONS_H
#define _OPERATIONS_H
/*
    operations.h
*/
#include <expression.h>
#include <outsider.h>


#ifndef IGNORE
#define IGNORE          0X000
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
#define ALOGICAL        0X800
#define AOPERATOR       (ACOMMA | ABASIC | ACOMPARE | ALOGICAL)

#define ALEAVE          (CLOSEBRACKET | ACONSTANT | AVARIABLE | APARAMETER)
#define NOTLEAVE        (OPENBRACKET | AOPERATOR)

#define HIGHEST 50


/* the below are used in expression.c */
extern Expression character_type[];     // character_type means it can be directly followed by any type
extern Expression opr_str_type[];       // opr_str_type means loaded as a string_type but is actually an operator
extern Expression string_type[];        // string_type means it must be followed by a space_type or a character_type

extern Expression number_type;          // is a number like 1, 23
extern Expression variable_type;        // is a user defined variable
extern Expression function_type;        // is a user defined function
extern Expression parameter_type;       // is a parameter to a user defined function
extern Expression userstring_type;      // is a user string, like for example a file name
extern Expression outsider_type;        // is a variable with name defined from outside
extern Expression current_type;         // is a variable used by the replacement operation
extern Expression contcall_type;

extern mchar Subscript[];
/* the above are used in expression.c */


extern void mfet_init(); // defined in operations.c

#define MIND(A,B) ((A==0 || B==0) ? 0 : (A==2 || B==2) ? 2 : 1)

#define INDEPENDENT(expression) \
    if(expression->independent==1) \
    {  expression->constant = out; \
       expression->evaluate = set_constant; \
       return set_constant(expression, argument); \
    }

#define NOT_YET \
{   set_error_message (CST21("Error in \\1 at \\2:\\3:\r\nOn '\\4': operation not yet available!"), expression->name); \
    return NULL; \
}


#endif /* _OPERATIONS_H */
