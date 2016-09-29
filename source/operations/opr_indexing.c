/*
    opr_indexing.c

    This file is #included by operations.c only.
*/

Value* opr_comma (Expression* expression, const Value* argument)
{
    Expression *expr;
    int i, n;
    long l, len=1;
    Value *vst, *out, *output=NULL;

    for(n=0, expr = expression->headChild; expr != NULL; expr = expr->nextChild, n++);
    Value** ptr = (Value**)_malloc(n*sizeof(Value*));

    for(i=0, expr = expression->headChild; expr != NULL; expr = expr->nextChild, i++)
    {
        vst = expression_evaluate(expr, argument);
        if(vst==NULL) break;
        len += VST_LEN(vst);
        ptr[i] = vst;
    }
    if(i==n)
    {
        output = out = value_alloc(len);
        setSepto2(*out, len);
        out++;
        for(i=0; i<n; i++)
        {   vst = ptr[i];
            l = VST_LEN(vst);
            memcpy(out, vst, l*sizeof(Value));
            vst->type=0; value_free(vst);
            out += l;
        }
    }
    else while(i--) value_free(ptr[i]);
    _free(ptr);
    return output;
}



Value* opr_concatenate (Expression* expression, const Value* argument) NOT_YET



typedef const Value* CValueP;
typedef struct _STA
{ CValueP n;
  Value* R;
} STA;

Value* do_indexing (const Value* in, const Value* I, const lchar* name, int info, Value CALL (Value, Value))
{
    long i, C, len;
    CValueP n, A, *array;
    Value v, *R;
    if(in==NULL) return NULL;

    if(!isSeptor(*in)) // error: cannot index a single number
    { set_error_message (TWST(Indexing_Single_Number), name); return NULL; }
    Value *out=NULL; 

    len = VST_LEN(in);
    for(C=0, A=in+1; A<in+len; A += VST_LEN(A)) C++;
    array = (CValueP*)_malloc(C*sizeof(CValueP));
    for(C=0, A=in+1; A<in+len; A += VST_LEN(A)) array[C++] = A;

    long t=0, outlen=0;
    len = VST_LEN(I);
    for(n=I; n<I+len; n++)
    {
        if(isSeptor(*n)) { outlen++; t++; continue; }
        v = toSma(*n); i = (long)getSmaInt(v);
        if(!isSmaInt(v)) { set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nIndex value is not an integer."), name); break; }
        if(i<0 || i>=C) { set_error_message (TWST(Index_OutOf_Range), name, TIS2(0,i), TIS2(1,C-1)); break; }
        A = array[i];
        outlen += VST_LEN(A);
    }
    if(n==I+len) // if no error
    {
      if(len==1) avaluecpy(&out, array[i]);
      else
      { out = R = value_alloc(outlen);
        STA* stack = (STA*)_malloc(t*sizeof(STA));
        t=-1;
        for(n=I; ; n++)
        {
            if(n==I+len || isSeptor(*n))
            {   if(t!=-1)
                {   while(n >= stack[t].n)
                    {   i = getSeptor(*(stack[t].R)).len;
                        if(--t==-1) break;
                        getSeptor(*(stack[t].R)).len += i;
                    }
                    if(t==-1) break;
                }
                t++;
                stack[t].n = n+VST_LEN(n);
                stack[t].R = R;
                setSepto2(*R,1);
                R++;
            }
            else
            {   i = (long)getSmaInt(toSma(*n));
                A = array[i]; // get at value structure to copy
                i = VST_LEN(A);
                vst_copy(R, A, i);
                R += i;
                getSeptor(*(stack[t].R)).len += i;
            }
        }
        _free(stack);
      }
    }
    _free(array);
    return out;
}
Value* opr_indexing (Expression* expression, const Value* argument) { return OPER2 (expression, argument, do_indexing, 0, 0); }



Value* do_generate_range (const Value* in, const lchar* name, int info, Value CALL (Value))
{
    const Value *A;
    Value v, fr, to, sum;
    long n, m;
    bool b;
    Value* B=NULL;
    Value* out=NULL;

    while(1) // not a loop
    {
        if(in==NULL || !check_first_level(in, 3, name)) break;

        fr = *(++in);  // skip Septor
        if(isSeptor(fr)) { set_error_message (TWST(Vector_Starting_Value), name); break; }

        A = ++in;   // skip 'fr' value
        m = VST_LEN(A);
        in += m;    // skip 'A' value
        if(m!=1) { m--; A++; } // skip Septor
        //TODO: check if A is a single value or vector
        B = value_alloc(m);

        setSmaInt(sum, 0);
        for(n=0; n<m; n++)
        {   B[n] = sum;
            sum = add(sum, A[n]);
        }
        if(getSmaInt(logical_not(sum))) { set_error_message (TWST(Vector_Middle_Value), name); break; }

        to = *in;
        if(isSeptor(to)) { set_error_message (TWST(Vector_Stopping_Value), name); break; }

        b = (bool)getSmaInt(lessOrEqual(fr, to));
        setSmaInt(v, 0);
        if(b) v = lessThan(sum, v);  // check if sum < 0
        else  v = lessThan(v, sum);  // check if sum > 0
        if(getSmaInt(v)) { set_error_message (TWST(Vector_Length_Invalid), name); break; }

        for(n=0; ; n++)
        {
            setSmaInt(v, n/m);
            v = multiply(v, sum);
            v = add(fr, v);
            v = add(v, B[n%m]);

            if(b) { if(!getSmaInt(lessOrEqual(v, to))) break; }
            else  { if(!getSmaInt(lessOrEqual(to, v))) break; }
        }

        Value* y;
        if(n==1) { out = value_alloc(1); y=out; }
        else { out = value_alloc(1+n); setSepto2(*out,1+n); y=out+1; }

        for(n=0; ; n++)
        {
            setSmaInt(v, n/m);
            v = multiply(v, sum);
            v = add(fr, v);
            v = add(v, B[n%m]);

            if(b) { if(!getSmaInt(lessOrEqual(v, to))) break; }
            else  { if(!getSmaInt(lessOrEqual(to, v))) break; }
            *y++ = v;
        }
        break;
    }
    value_free(B);
    return out;
}
Value* generate_range  (Expression* expression, const Value* argument) { return OPER1 (expression, argument, do_generate_range , 0, 0); }



Value* do_generate_vector (const Value* in, const lchar* name, int info, Value CALL (Value))
{
    const Value *A;
    Value v, fr, sum;
    long n, m;
    Value* B=NULL;
    Value* out=NULL;

    while(1) // not a loop
    {
        if(in==NULL || !check_first_level(in, 3, name)) break;

        fr = *(++in);  // skip Septor
        if(isSeptor(fr)) { set_error_message (TWST(Vector_Starting_Value), name); break; }

        A = ++in;   // skip 'fr' value
        m = VST_LEN(A);
        in += m;    // skip 'A' value
        if(m!=1) { m--; A++; } // skip Septor
        //TODO: check if A is a single value or vector
        B = value_alloc(m);

        setSmaInt(sum, 0);
        for(n=0; n<m; n++)
        {   B[n] = sum;
            sum = add(sum, A[n]);
        }

        v = *in;
        long len = isSmaInt(v) ? (long)getSmaInt(v) : 0;
        if(len<1) { set_error_message (TWST(Vector_Length_Invalid), name); break; }
        n=len;

        Value* y;
        if(n==1) { out = value_alloc(1); y=out; }
        else { out = value_alloc(1+n); setSepto2(*out,1+n); y=out+1; }

        for(n=0; n<len; n++)
        {
            setSmaInt(v, n/m);
            v = multiply(v, sum);
            v = add(fr, v);
            v = add(v, B[n%m]);
            *y++ = v;
        }
        break;
    }
    value_free(B);
    return out;
}
Value* generate_vector (Expression* expression, const Value* argument) { return OPER1 (expression, argument, do_generate_vector, 0, 0); }

