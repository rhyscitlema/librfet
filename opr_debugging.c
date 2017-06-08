/*
    opr_debugging.c
*/



bool size_of_value (ExprCallArg eca)
{
    int rows, cols;
    Expression* expression = eca.expression;
    Expression* head = expression->headChild;
    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;

    // check if child node is of matrix value structure
    if(!valueSt_matrix_getSize (eca.stack, &rows, &cols))
    {
        wchar* errormessage = eca.garg->message;
        VstToStr(eca.stack, ErrStr0, 1|4,-1,-1,-1);
        set_message(errormessage, TWST(Operand_IsNot_Matrix), expression->name, ErrStr0);
        return 0;
    }
    eca.stack[0] = setSepto2(3, 2);
    eca.stack[1] = setSmaInt(rows);
    eca.stack[2] = setSmaInt(cols);

    if(expression->independent==0) // if first time
        INDEPENDENT(expression, eca.stack, head->independent);
    return 1;
}



bool print_value (ExprCallArg eca)
{
    lchar c = *eca.expression->name;
    eca.expression = eca.expression->headChild;
    if(!expression_evaluate(eca)) return 0;
    wchar* output = eca.garg->message;
    sprintf2 (output, L"[%s:%s:%s] ", TWSF(Print_Value), TIS2(0,c.line), TIS2(1,c.coln));
    VstToStr (eca.stack, output + strlen2(output), 4, -1, -1, -1);
    return 0; // NOTE: the print is always as an error message
}



bool check_arguments_count (Expression* expression, int Count, wchar* errormessage)
{
    if(expression->info) return 1;
    expression->info = 1;
    int count;
    Expression* expr = expression->headChild;

    if(expr->type & ACOMMA) // check if a ',' node
    {   // count the number of arguments
        for(count=0, expr = expr->headChild; expr != NULL; expr = expr->nextSibling) count++;
    }
    else count = 1;

    if(count != Count) { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\nExpect \\5 arguments but \\6 found.", expression->name, TIS2(0,Count), TIS2(1,count)); return false; }
    return true;
}



/* start evaluating from first argument and return first non-NULL result */
bool try_and_catch (ExprCallArg eca)
{
    Expression *expr = eca.expression->headChild;
    if(expr->ID == CommaSeparator)
        expr = expr->headChild;
    for( ; expr != NULL; expr = expr->nextSibling)
    {
        eca.expression = expr;
        if(expression_evaluate(eca)) return 1;
        // TODO: try_and_catch(): do resolve independence?
    }
    return 0;
}



bool convert_to_str (ExprCallArg eca)
{
    Expression* expression = eca.expression;
    eca.expression = expression->headChild;
    if(!expression_evaluate(eca)) return 0;

    // set default values
    int info=0;
    char base=-1;
    char digits=-1;
    short dplaces=-1;
    wchar* mstr = eca.garg->message;

    value* in = eca.stack;
    if(!check_first_level(in, 2, mstr, expression->name)) return 0;
    in++;
    long len = VST_LEN(in);

    if(len==1) base = (char)getSmaInt(_floor(in[0]));
    else if(len==3)
    {
        info    = (int )getSmaInt(_floor(in[1]));
        base    = (char)getSmaInt(_floor(in[2]));
      //digits  = (char)getSmaInt(_floor(in[3]));
      //dplaces =(short)getSmaInt(_floor(in[4]));
    }
    else { set_message(mstr, L"Error in \\1 at \\2:\\3 on '\\4':\r\nFirst argument must be 1 or 2 values: (info, base).", expression->name); return 0; }

    // get the value to be converted to string
    in += len;
    if(!VstToStr(in, mstr, info, base, digits, dplaces)) return 0;

    lchar* lstr=NULL;
    astrcpy32(&lstr, mstr);
    *eca.stack = setString(lstr);
    return 1;
}



bool convert_to_num (ExprCallArg eca) NOT_YET

