#ifndef _EXPRESSION_H
#define _EXPRESSION_H
/*
	expression.h
*/

#include "structures.h"


/*
	Defined in expression.c and used by
	expression.c and component.c only.
*/
const Expression* get_next_item (
	value stack,
	const_Str3* strExpr,
	const_Str3* str,
	const Expression *currentItem,
	Component *const component);


/*
	The function 'name' below is perhaps the only veteran
	to the whole calculator software source code. It has
	been there ever since the very first implementation,
	and has survived all the countless code revisions.
	When will it finally go down? I wonder!
	It all started with the discovery of a new parsing algorithm:
	http://rhyscitlema.com/algorithms/expression-parsing-algorithm
*/
value parseExpression (value stack, const_Str3 strExpr, Component *component);


typedef struct _OperEval { // NOTE: never change this.
	uint8_t  paras;  // number of parameters to function call
	uint8_t  input;  // user input ID used for outsider calls
	//uint16_t recurs; // number of recursive calls (union with:
	uint16_t start;  // relative offset to start of evaluation)
	uint32_t result; // -ve offset to final location of result
	uint32_t stack;  // -ve offset to previous stack pointer
	uint32_t p_try;  // -ve offset to previous 'try' pointer
	const_value opers; // pointer to previous opers array
	Container* caller; // pointer to initial caller container
} OperEval; // see component_evaluate() in component.c
#define OperEvalSize (sizeof(OperEval)/sizeof(uint32_t))


/*
	Note: the caller must pre-set the required data structure
	relative to the given <stack> pointer parameter. Refer to
	the SET_VAR_FUNC part of the function's implementation to
	see what exactly the required data structure is. A usual
	thing to do is just: memset(stack, 0, sizeof(OperEval));
*/
value operations_evaluate (value stack, const_value oper);


#endif
