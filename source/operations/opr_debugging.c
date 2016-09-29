/*
    opr_debugging.c
*/



Value* get_size (const Value* in, const lchar* name, int info, Value CALL (Value))
{
    if(in==NULL) return NULL;
    Value* out=NULL;
    int rows, cols;

    // check if child node is of matrix value structure
    if(!valueSt_matrix_getSize (in, &rows, &cols))
    {
        VstToStr(in, error_message+1000, 1|4,-1,-1,-1);
        set_error_message (TWST(Operand_IsNot_Matrix), name, error_message+1000);
    }
    else
    {   out = value_alloc(3);
        setSepto2(out[0], 3);
        setSmaInt(out[1], rows);
        setSmaInt(out[2], cols);
    }
    return out;
}
Value* size_of_value (Expression* expression, const Value* argument) { return OPER1 (expression, argument, get_size, 0, 0); }



Value* print_value (Expression* expression, const Value* argument)
{
    Value* in = expression_evaluate(expression->headChild, argument);
    if(in==NULL) return NULL;
    sprintf2 (error_message, CST21("[%s] "), TWSF(Print_Value));
    VstToStr (in, error_message + strlen2(error_message), 4, -1, -1, -1);
    value_free(in);
    return NULL; // NOTE: the print is always as an error_message
}



bool check_arguments_count (Expression* expression, int Count)
{
    if(expression->info) return 1;
    expression->info = 1;
    int count;
    Expression* expr = expression->headChild;

    if(expr->type & ACOMMA) // check if a ',' node
    {   // count the number of arguments
        for(count=0, expr = expr->headChild; expr != NULL; expr = expr->nextChild) count++;
    }
    else count = 1;

    if(count != Count) { set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nExpect \\5 arguments but \\6 found."), expression->name, TIS2(0,Count), TIS2(1,count)); return false; }
    return true;
}

bool check_first_level (const Value* in, int Count, const lchar* name)
{
    if(!in) return 0;
    int count;
    const Value* end = in + VST_LEN(in);
    if(in+1!=end)
    {   // count the number of arguments
        for(count=0, in++; in < end; in += VST_LEN(in)) count++;
    }
    else count=1;
    if(count!=Count) { set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nExpect \\5 arguments but \\6 found."), name, TIS2(0,Count), TIS2(1,count)); return 0; }
    return 1;
}



/* start evaluating from first argument and return first non-NULL result */
Value* try_and_catch (Expression* expression, const Value* argument)
{
    Expression *expr = expression->headChild;
    if(expr->evaluate == opr_comma)
        expr = expr->headChild;
    for( ; expr != NULL; expr = expr->nextChild)
    {   Value* out = expression_evaluate(expr, argument);
        // TODO: try_and_catch(): resolve independence
        if(out) return out;
    }
    return NULL;
}



Value* do_to_str (const Value* in, const lchar* name, int info, Value CALL (Value))
{
    // set default values
    char more=0;
    char base=-1;
    char digits=-1;
    short dplaces=-1;

    if(!check_first_level(in, 2, name)) return NULL;
    in++;
    long len = VST_LEN(in);

    if(len==1) base = (char)toSmaInt(in[0], base);
    else if(len==3)
    {
        more    = (char)toSmaInt(in[1], more);
        base    = (char)toSmaInt(in[2], base);
      //digits  = (char)toSmaInt(in[3], digits);
      //dplaces =(short)toSmaInt(in[4], dplaces);
    }
    else { set_error_message(CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nFirst argument must be 1 or 2 values: (info, base)."), name); return NULL; }

    // get the value to be converted to string
    in += len;
    if(!VstToStr(in, error_message, more, base, digits, dplaces)) return NULL;

    Value* out = value_alloc(1);
    lchar* lstr=NULL;
    astrcpy32(&lstr, error_message);
    setString(*out, lstr);
    return out;
}
Value* convert_to_str (Expression* expression, const Value* argument) { return OPER1 (expression, argument, do_to_str, 0, 0); }



Value* convert_to_num (Expression* expression, const Value* argument) NOT_YET

