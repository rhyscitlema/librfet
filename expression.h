#ifndef _EXPRESSION_H
#define _EXPRESSION_H
/*
    expression.h
*/

#include "structures.h"


/* Defined in expression.c and used by
   expression.c and component.c only.
*/
const Expression* get_next_item (
                const lchar** strExpr,
                lchar** str,
                const Expression *currentItem,
                Component *component);


/* The function name below is perhaps the only VETERAN
   to the whole calculator software source code. Essentially,
   it has been there ever since the very first implementation,
   and has survived all the countless source code revisions!
   When will it finally go down... I wonder...?!!
   This all started with the discovery of a new parsing algorithm:
   http://rhyscitlema.com/algorithms/expression-parsing-algorithm
*/
Expression *parseExpression (const lchar* strExpr, Component *component);

void expression_tree_print (const Expression *expression);


/* Called only at the end of a successfull evaluation.
   Placed here only so to be available to outsider.c.
*/
void INDEPENDENT (Expression* expression, value* stack, int independent);


static inline bool check_first_level (const value* in, int Cols, wchar* errormessage, const lchar* name)
{
    value v;
    int cols;

    if(!in) return 0;
    v = *in;
    cols = getType(v)==aSeptor ? getSeptor(v).cols : 1;

    if(cols!=Cols)
    {
        set_message( errormessage,
            L"Error in \\1 at \\2:\\3 on '\\4':\r\nExpect \\5 arguments but \\6 found.",
            name, TIS2(0,Cols), TIS2(1,cols));
        return 0;
    }
    else return 1;
}


#endif

