/*
    opr_other.c

    This file is #included by operations.c only.
*/



/* Here we have something like ( (condition) ? (if true) : (if false) )
*/
Value* opr_condition (Expression* expression, const Value* argument)
{
    Expression *head, *last;
    head = expression->headChild;
    last = expression->lastChild;

    if(expression->name->mchr == ':') { set_error_message (TWST(Expect_Question_Before), expression->name); return NULL; }
    if(last->name->mchr != ':') { set_error_message (TWST(Expect_Choice_After), expression->name); return NULL; }

    Value* out = expression_evaluate(head, argument);
    if(out==NULL) return NULL;

    long len = VST_LEN(out);
    if(len != 1) { set_error_message (TWST(Condition_IsNot_Single), expression->name, TIS2(0,len)); value_free(out); return NULL; }

    last = (!getSmaInt(logical_not(*out)) ? last->headChild : last->lastChild);
    value_free(out);
    out = expression_evaluate(last, argument);
    return out;
}



/* Here we have something like ((0,0,0) := MouseMotion)
*/
Value* opr_replace (Expression* expression, const Value* argument)
{
    Expression *head, *last;
    head = expression->headChild;
    last = expression->lastChild;
    Value* out = head->constant;

    if(out==NULL)
    {
        out = expression_evaluate(head, argument);
        if(out==NULL) return NULL;
        if(head->constant==NULL)
           head->constant = out;
        else
        {  value_free(out);
           out = head->constant;
        }
    }
    if(expression->component->container == replacement_container
    && expression->info != evaluation_instance)
    {
        Value* current = out;
        out = expression_evaluate(last, argument);
        if(out==NULL) return NULL;

        if(0!=value_compare(current, out))
        {
            value_free(current);
            head->constant = out;
            out = value_copy(out);
            replacement_record(replacement_container, expression);
        }
    } else out = value_copy(out);
    expression->info = evaluation_instance;
    return out;
    // store it as a deletable or non-deletable constant,
    // but however make sure to return it as non-deletable.
}



/* Here we have something like ((0,0,0) := current)
*/
Value* set_current (Expression* expression, const Value* argument)
{
    Expression *expr;
    if(expression->headChild==NULL)
    {
        expression->independent = 2;
        for(expr = expression->parent; expr != NULL; expr = expr->parent)
            if(expr->evaluate == opr_replace) break;
        expression->headChild = expr->headChild;
    }
    return value_copy(expression->headChild->constant);
}



Value* do_fullfloor (const Value* in, const lchar* name, int info, Value CALL (Value))
{
    long i, n, m, len;
    const Value *N, *A;
    Value y, sum, numerator;

    if(in==NULL || !check_first_level(in, 2, name)) return NULL;

    N = in+1;           // get vector of values of n
    len = VST_LEN(N);
    Value* output = value_alloc(len);
    Value* out = output;

    A = N + len;          // get the vector A
    m = VST_LEN(A);
    if(m>1) { m--; A++; } // skip ',' separator
    //TODO: check if A is a single value or vector

    setSmaInt(sum,0);
    for(i=0; i<m; i++) sum = add(sum, A[i]);

    if(getSmaInt(logical_not(sum))) // if sum==0
    {
        for(n=len; n!=0; n--, N++, out++)
        {
            y = *N;
            if(!isSeptor(y))
                setSmaInt(y, (SmaInt)0x8000000000000000);
            *out = y;
        }
    }
    else
    {   for(n=len; n!=0; n--, N++, out++)
        {
            y = *N;
            if(!isSeptor(y))
            {   numerator = y;
                setSmaInt(y, 0);
                for(i=1; i<=m; i++)
                {
                    y = add(y, idivide(numerator, sum));
                    numerator = add (numerator, A[m-i]);
                }
            }
            *out = y;
        }
    }
    return output;
}
Value* fct_fullfloor (Expression* expression, const Value* argument) { return OPER1 (expression, argument, do_fullfloor, 0, 0); }



Value* fct_rand (Expression* expression, const Value* argument) NOT_YET

Value* fct_srand (Expression* expression, const Value* argument) NOT_YET

Value* fct_getprimes (Expression* expression, const Value* argument) NOT_YET

