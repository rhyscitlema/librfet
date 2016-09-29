#ifndef _EXPRESSION_H
#define _EXPRESSION_H
/*
    expression.h
*/

#include "structures.h"


const Expression* get_next_item (
                const lchar** strExpr,
                lchar** str,
                const Expression *currentItem,
                const Value* parameter,
                Component *component,
                Container *container);

Expression *parseExpression (const lchar* strExpr,
                             const Value* parameter,
                             Component *component,
                             Container *container);

void expression_tree_print (const Expression *expression);


/* also used in parseExpression by expression.c */
Value* opr_replace (Expression* expression, const Value* argument);

/* also used in graphplotter3d by graph_edit() */
Value* opr_equal_type4 (Expression* expression, const Value* argument);
Value* opr_comma       (Expression* expression, const Value* argument);
Value* set_function    (Expression* expression, const Value* argument);
Value* set_userstring  (Expression* expression, const Value* argument);


#endif

