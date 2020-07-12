#ifndef _OPERATIONS_H
#define _OPERATIONS_H
/*
	operations.h
*/
#include "structures.h"

void operations_init (value stack);


enum OPER_INFO_MASK
{
	SkipClimbUp     = 0x001,
	RightAssoct     = 0x002,
	OPENBRACKET     = 0x004,
	CLOSEBRACKET    = 0x008,

	ACONSTANT       = 0x010,
	AVARIABLE       = 0x020,
	AFUNCTION       = 0x040,
	APARAMTER       = 0x080,

	ACOMMA          = 0x100,
	ABASIC          = 0x200,
	ACOMPARE        = 0x400,
	ALOGICAL        = 0x800,
	AOPERATOR       = (ACOMMA | ABASIC | ACOMPARE | ALOGICAL),

	ALEAVE          = (CLOSEBRACKET | ACONSTANT | AVARIABLE | APARAMTER),
	NOTLEAVE        = (OPENBRACKET | AOPERATOR),

	HIGHEST = 50    // highest operation precedence level
};


/* the below are used in operations.c and expression.c */

extern Expression char_type[80];   // char_type means it can be directly followed by any type (ex: +)
extern Expression oper_type[10];   // oper_type means loaded as a word_type although is an operator (ex: mod)
extern Expression word_type[70];   // word_type means it must be inbetween space_type or char_type (ex: pi)

extern Expression constant_type;       // is a literal constant like 1, 23, 'c', "word"
extern Expression compname_type;       // is a string obtained from component_path_name()
extern Expression outsider_type;       // is a component defined from an outside program
extern Expression repl_lhs_type;       // is a variable used by the replacement operator
extern Expression paramter_type;       // is a parameter to a user defined function
extern Expression var_func_type;       // is a call to user defined variable or function
extern Expression dot_call_type;       // is a '.' used for container.component call


#endif /* _OPERATIONS_H */
