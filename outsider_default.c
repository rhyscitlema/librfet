/*
    outsider_default.c

    Note:
    I am needed by the MFET_Calculator library.
    There is no need to compile me independently.
    Just #include me anywhere in main.c file.
    I am essentially just a 'template'.
    By default, the only outsider I have is time 't'.
*/

#include <outsider.h>


bool user_input_allowed = false;


enum IDs
{
    NONE=0,
    TIME,
    END
};


/* Return outsider's ID > 0, else return 0.
   Time 't' is also considered an outsider! */
unsigned int outsider_getID (const lchar* lstr)
{
    if(0==strcmp31(lstr, "t")) return TIME;
    return NONE;
}


/* Evaluate outsider */
Value* set_outsider (Expression* expression, const Value* argument)
{
    Value* out;
    SmaFlt value[10];
    int rows=1, cols=1;

    //Expression *head = expression->headChild;
    //Value* in = expression_evaluate(head, argument);

    switch(expression->ID)
    {
    case TIME:
        value[0]=0;
        break;

    default:
        set_error_message (CST21("Software Error: in '\\1' at \\2:\\3:\r\nOn '\\4': outsider is unrecognised."), expression->name);
        return NULL;
    }

    out = value_alloc(1+rows*(1+cols));
    valueSt_from_floats (out, rows, cols, value);
    return out;
}
