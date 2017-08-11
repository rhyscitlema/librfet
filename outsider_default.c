/*
    outsider_default.c

    Note:
    I am needed by librfet.
    There is no need to compile me independently,
    just #include me anywhere in main.c file.
    I am essentially just a 'template'.
    By default, the only outsider I have is 'time'.
*/

#include <outsider.h>


bool user_input_allowed = false;


enum OUTSIDER_ID
{
    NONE=0,
    TIME,
    END
};



/* Get Outsider ID */
#define GOID(string, ID) if(0==strcmp31(lstr, string)) return ID;


/* Return outsider's ID > 0, else return 0. */
unsigned int outsider_getID (const lchar* lstr)
{
    GOID ("time", TIME)
    return NONE;
}


/* Evaluate outsider expression */
bool set_outsider (ExprCallArg eca)
{
    int rows=1, cols=1;

    int id = GET_OUTSIDER_ID(eca.expression);
    switch(id)
    {
    case TIME:
        eca.stack[0] = setSmaInt(0);
        break;

    default:
        set_message (eca.garg->message, L"Software Error: in \\1 at \\2:\\3:\r\nOn '\\4': outsider \\5 is unrecognised.", eca.expression->name, TIS2(0,id));
        return 0;
    }

    valueSt_matrix_setSize (eca.stack, rows, cols);
    return 1;
}


/* Called by component_parse() */
bool checkfail (const Container* c, const lchar* name) { return false; }

