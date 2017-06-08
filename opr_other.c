/*
    opr_other.c

    This file is #included by operations.c only.
*/



/* Here we have something like ( (condition) ? (if true) : (if false) )
*/
bool opr_condition (ExprCallArg eca)
{
    Expression* expression = eca.expression;
    Expression* head = expression->headChild;
    Expression* last = expression->lastChild;

    wchar* errmsg = eca.garg->message;
    if(expression->ID != ConditionAsk) { set_message(errmsg, TWST(Expect_Question_Before), expression->name); return 0; }
    if(last->ID != ConditionChoose) { set_message(errmsg, TWST(Expect_Choice_After), expression->name); return 0; }

    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;

    long len = VST_LEN(eca.stack);
    if(len != 1) { set_message(errmsg, TWST(Condition_IsNot_Single), expression->name, TIS2(0,len)); return 0; }

    last = (!getSmaInt(logical_not(*eca.stack)) ? last->headChild : last->lastChild);
    eca.expression = last;
    if(!expression_evaluate(eca)) return 0;

    if(expression->independent==0) // if first time
        INDEPENDENT(expression, eca.stack, MIND(head->independent, last->independent));
    return 1;
}



/* Here we have something like ((0,0,0) := MouseMotion)
*/
bool opr_replace (ExprCallArg eca)
{
    Expression* expression = eca.expression;
    Expression* head = expression->headChild;

    if(!getType(head->constant)) // if very first time
    {
        eca.expression = head;
        if(!expression_evaluate(eca)) return 0;
        assert(!getType(head->constant));
        head->constant = toSingle(eca.stack);
    }
    if(c_container(expression->component) == replacement_container
    && expression->info != evaluation_instance)
    {
        eca.expression = expression->lastChild;
        if(!expression_evaluate(eca)) return 0;

        bool b = isPoiter(head->constant);
        value* current = b ? getPoiter(head->constant) : &head->constant;

        if(0!=value_compare(current, eca.stack))
        {
            if(b) value_free(current);
            head->constant = toSingle(eca.stack);
            replacement_record(replacement_container, expression);
        }
    }
    else
    {   eca.expression = head;
        set_constant(eca);
    }
    expression->info = evaluation_instance;
    return 1;
}



/* Here we have something like ((0,0,0) := current)
*/
bool set_current (ExprCallArg eca)
{
    // NOTE: using ->headChild makes expression_remove() to crash
    Expression *lhs = eca.expression->lastChild; // Left Hand Side
    if(lhs==NULL)
    {
        eca.expression->independent = 2;
        for(lhs = eca.expression->parent; lhs != NULL; lhs = lhs->parent)
            if(lhs->ID == Replacement) break;
        lhs = eca.expression->lastChild = lhs->headChild;
    }
    eca.expression = lhs;
    return set_constant(eca);
}



bool fct_fullfloor (ExprCallArg eca)
{
    long i, n, m, len;
    const value *N, *A;
    value y, sum, numerator;

    Expression* expression = eca.expression;
    eca.expression = expression->headChild;
    if(!expression_evaluate(eca)) return 0;

    wchar* errmsg = eca.garg->message;
    if(!check_first_level(eca.stack, 2, errmsg, expression->name)) return 0;

    N = eca.stack+1;          // get vector of values of n
    len = VST_LEN(N);

    A = N + len;          // get the vector A
    m = VST_LEN(A);
    if(m>1) { m--; A++; } // skip ',' separator
    //TODO: check if A is a single value or vector

    sum = setSmaInt(0);
    for(i=0; i<m; i++) sum = add(sum, A[i]);

    if(getSmaInt(logical_not(sum))) // if sum==0
    {
        for(n=len; n!=0; n--, N++, eca.stack++)
        {
            y = *N;
            if(!isSeptor(y))
                y = setSmaInt((SmaInt)0x8000000000000000);
            *eca.stack = y;
        }
    }
    else
    {   for(n=len; n!=0; n--, N++, eca.stack++)
        {
            y = *N;
            if(!isSeptor(y))
            {
                numerator = y;
                y = setSmaInt(0);
                for(i=1; i<=m; i++)
                {
                    y = add(y, idivide(numerator, sum));
                    numerator = add (numerator, A[m-i]);
                }
            }
            *eca.stack = y;
        }
    }
    return 1;
}



bool fct_rand (ExprCallArg eca) NOT_YET

bool fct_srand (ExprCallArg eca) NOT_YET

bool fct_getprimes (ExprCallArg eca) NOT_YET

