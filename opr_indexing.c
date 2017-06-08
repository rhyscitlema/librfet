/*
    opr_indexing.c

    This file is #included by operations.c only.
*/

bool opr_comma (ExprCallArg eca)
{
    value* stack = eca.stack;
    long c=0, len=1;
    Expression *expr = eca.expression->headChild;

    for( ; expr != NULL; expr = expr->nextSibling, c++)
    {
        eca.expression = expr;
        eca.stack = stack+len;
        if(!expression_evaluate(eca)) return 0;
        len += VST_LEN(eca.stack);
    }
    *stack = setSepto2(len, c);
    return 1;
}



bool opr_concatenate (ExprCallArg eca) // TODO: fix operator precedence
{
    long headLen, lastLen, cols;
    value* start = eca.stack;
    value v;

    Expression* expression = eca.expression;
    Expression* head = expression->headChild;
    Expression* last = expression->lastChild;

    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;
    headLen = VST_LEN(eca.stack);

    if(headLen==1)
    {   headLen++;
        cols=1;
        v = eca.stack[0];
        eca.stack++;
    }
    else
    {   cols = getSeptor(*eca.stack).cols;
        eca.stack += headLen-1;
        v = *eca.stack;
    }

    eca.expression = last;
    if(!expression_evaluate(eca)) return 0;
    lastLen = VST_LEN(eca.stack);

    if(lastLen==1)
    {   headLen++; 
        cols++;
        eca.stack[1] = eca.stack[0];
    }
    else
    {   headLen += lastLen-1;
        cols += getSeptor(*eca.stack).cols;
    }
    eca.stack[0] = v;
    *start = setSepto2(headLen, cols);
    return 1;
}



static inline long get_index (value v, long cols, wchar* errmsg, const lchar* name)
{
    if(!isSmaInt(v)) { set_message(errmsg, L"Error in \\1 at \\2:\\3 on '\\4':\r\nIndex value is not an integer.", name); return -1; }
    long i = getSmaInt(v);
    if(i<0 || i>=cols) { set_message(errmsg, TWST(Index_OutOf_Range), name, TIS2(0,i), TIS2(1,cols-1)); return -1; }
    return i;
}

typedef struct _Array { value *beg, *end; } Array;

bool opr_indexing (ExprCallArg eca)
{
    long i, t, cols;
    value v, *n, *R, *out;
    Array *array, *S;
    long headLen, lastLen;
    value *headVst, *lastVst;
    value* stack = eca.stack;

    Expression* expression = eca.expression;
    Expression* head = expression->headChild;
    Expression* last = expression->lastChild;

    headVst = eca.stack;
    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;
    headLen = VST_LEN(headVst);

    lastVst = headVst + headLen;
    eca.expression = last;
    eca.stack = lastVst;
    if(!expression_evaluate(eca)) return 0;
    lastLen = VST_LEN(lastVst);

    wchar* errmsg = eca.garg->message;

    if(lastLen==1)
    {
        if(headLen==1) cols = 1;
        else cols = getSeptor(*headVst).cols;
        i = get_index(*lastVst, cols, errmsg, expression->name);
        if(i<0) return 0;

        if(headLen!=1)
        {
            for(n = headVst+1; i!=0; i--, n += VST_LEN(n));
            vst_shift(stack, n);
        }
    }
    else // TODO: in opr_indexing(): there are still errors
    {
        array = (Array*)(lastVst + lastLen);
        for(cols=0, n = headVst+1; n < headVst+headLen; n += VST_LEN(n)) array[cols++].beg = n;

        S = array + cols;
        out = (value*)(S + cols);
        R = out;
        t = -1;

        for(n = lastVst; ; n++)
        {
            v = *n;
            if(n==lastVst+lastLen || isSeptor(v))
            {   if(t!=-1)
                {   while(n >= S[t].beg)
                    {   i = getSeptor(*(S[t].end)).len;
                        if(--t==-1) break;
                        Septor sep = getSeptor(*(S[t].end));
                        sep.len += i;
                        *(S[t].end) = setSeptor(sep);
                    }
                    if(t==-1) break;
                }
                t++;
                S[t].beg = n + getSeptor(v).len;
                S[t].end = R;
                *R = setSepto2(1, getSeptor(v).cols);
                R++;
            }
            else
            {
                i = get_index(v, cols, errmsg, expression->name);
                if(i<0) return 0;
                headVst = array[i].beg; // get at value structure to copy
                i = VST_LEN(headVst);
                vst_shift(R, headVst);
                R += i;
                Septor sep = getSeptor(*(S[t].end));
                sep.len += i;
                *(S[t].end) = setSeptor(sep);
            }
        }
        vst_shift(stack, out);
    }
    return 1;
}



bool generate_range (ExprCallArg eca)
{
    long n, m;
    value v, fr, to, sum;
    value *A, *out;

    Expression* expression = eca.expression;
    eca.expression = expression->headChild;
    if(!expression_evaluate(eca)) return 0;
    wchar* errmsg = eca.garg->message;

    value* stack = eca.stack;
    if(!check_first_level(stack, 3, errmsg, expression->name)) return 0;
    stack++;        // skip Septor

    fr = *stack;    // get starting value
    if(isSeptor(fr)) { set_message(errmsg, TWST(Vector_Starting_Value), expression->name); return 0; }
    stack++;        // skip 'fr' value

    A = stack;      // get increment value
    m = VST_LEN(A);
    stack += m;     // skip 'A' value

    if(m!=1) { m--; A++; } // skip Septor
    //TODO: check if A is a single value or vector

    sum = setSmaInt(0);
    for(n=0; n<m; n++)
    {   v = A[n];
        A[n] = sum;
        sum = add(sum, v);
    }

    to = *stack;     // get stopping value
    if(isSeptor(to)) { set_message(errmsg, TWST(Vector_Stopping_Value), expression->name); return 0; }
    stack++;        // skip 'to' value

    bool b = (bool)getSmaInt(lessOrEqual(fr, to));
    v = setSmaInt(0);
    if(b) v = lessThan(sum, v);  // check if sum < 0
    else  v = lessThan(v, sum);  // check if sum > 0
    if(getSmaInt(v)) { set_message(errmsg, TWST(Vector_Length_Invalid), expression->name); return 0; }

    out = stack;    // prepare for output
    out++;          // skip output Septor

    for(n=0; ; n++)
    {
        v = setSmaInt(n/m);
        v = multiply(v, sum);
        v = add(fr, v);
        v = add(v, A[n%m]);

        if(b) { if(!getSmaInt(lessOrEqual(v, to))) break; }
        else  { if(!getSmaInt(lessOrEqual(to, v))) break; }
        *out++ = v;
    }
    if(n==1) *eca.stack = stack[1];
    else
    {   *stack = setSepto2(1+n, n);
        vst_shift(eca.stack, stack);
    }
    return 1;
}



bool generate_vector (ExprCallArg eca)
{
    long n, m, len;
    value v, fr, sum;
    value *A, *out;

    Expression* expression = eca.expression;
    eca.expression = expression->headChild;
    if(!expression_evaluate(eca)) return 0;
    wchar* errmsg = eca.garg->message;

    value* stack = eca.stack;
    if(!check_first_level(stack, 3, errmsg, expression->name)) return 0;
    stack++;        // skip Septor

    fr = *stack;    // get starting value
    if(isSeptor(fr)) { set_message(errmsg, TWST(Vector_Starting_Value), expression->name); return 0; }
    stack++;        // skip 'fr' value

    A = stack;      // get increment value
    m = VST_LEN(A);
    stack += m;     // skip 'A' value

    if(m!=1) { m--; A++; } // skip Septor
    //TODO: check if A is a single value or vector

    sum = setSmaInt(0);
    for(n=0; n<m; n++)
    {   v = A[n];
        A[n] = sum;
        sum = add(sum, v);
    }

    v = *stack;     // get vector length
    len = isSmaInt(v) ? getSmaInt(v) : 0;
    if(len<1) { set_message(errmsg, TWST(Vector_Length_Invalid), expression->name); return 0; }
    stack++;        // skip 'v' value

    out = stack;    // prepare for output
    out++;          // skip output Septor

    for(n=0; n<len; n++)
    {
        v = setSmaInt(n/m);
        v = multiply(v, sum);
        v = add(fr, v);
        v = add(v, A[n%m]);
        *out++ = v;
    }
    if(n==1) *eca.stack = stack[1];
    else
    {   *stack = setSepto2(1+n, n);
        vst_shift(eca.stack, stack);
    }
    return 1;
}

