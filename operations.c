/*
    operations.c
*/

#include "operations.h"
#include "expression.h"
#include "component.h"
#include "outsider.h"
#include <_stdio.h>


Expression char_type[80];   // char_type means it can be directly followed by any type (ex: +)
Expression oper_type[10];   // oper_type means loaded as a word_type although is an operator (ex: mod)
Expression word_type[70];   // word_type means it must be inbetween space_type or char_type (ex: pi)

Expression constant_type;       // is a literal constant like 1, 23, 'c', "word"
Expression compname_type;       // is a string obtained from component_path_name()
Expression outsider_type;       // is a component defined from an outside program
Expression repl_lhs_type;       // is a variable used by the replacement operator
Expression paramter_type;       // is a parameter to a user defined function
Expression var_func_type;       // is a call to user defined variable or function
Expression dot_call_type;       // is a '.' used for container.component call


static void operation_add( Expression *expr, int ID, int type, int previous, int precedence)
{
    if(!expr) return;
    memset(expr, 0, sizeof(Expression));
    expr->ID = ID;
    expr->type = type;
    expr->previous = previous;
    expr->precedence = precedence;
}


static void operation_load_comp_type()
{
    operation_add( &constant_type  , SET_CONSTANT  , SkipClimbUp | ACONSTANT  , NOTLEAVE , HIGHEST );
    operation_add( &compname_type  , SET_COMPNAME  , SkipClimbUp | ACONSTANT  , NOTLEAVE , HIGHEST );
    operation_add( &outsider_type  , SET_OUTSIDER  , SkipClimbUp | AVARIABLE  , NOTLEAVE , HIGHEST );
    operation_add( &repl_lhs_type  , SET_REPL_LHS  , SkipClimbUp | AVARIABLE  , NOTLEAVE , HIGHEST );
    operation_add( &paramter_type  , SET_PARAMTER  , SkipClimbUp | APARAMTER  , NOTLEAVE , HIGHEST );
    operation_add( &var_func_type  , SET_VAR_FUNC  , SkipClimbUp | AFUNCTION  , NOTLEAVE , HIGHEST );
    operation_add( &dot_call_type  , SET_DOT_CALL  , 0 , ALEAVE , HIGHEST-1 ); // -1 because it is an operator
}


static void operation_load_char_type()
{
    int i=0;

    operation_add( &char_type[i++] , OpenedBracket1 , SkipClimbUp | OPENBRACKET  , NOTLEAVE | AFUNCTION , 1 );
    operation_add( &char_type[i++] , OpenedBracket2 , SkipClimbUp | OPENBRACKET  , NOTLEAVE     ,  1  );
    operation_add( &char_type[i++] , OpenedBracket3 , SkipClimbUp | OPENBRACKET  , NOTLEAVE     ,  1  );
    operation_add( &char_type[i++] , ClosedBracket1 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2  );
    operation_add( &char_type[i++] , ClosedBracket2 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2  );
    operation_add( &char_type[i++] , ClosedBracket3 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2  );

    operation_add( &char_type[i++] , Replacement    , RightAssoct | ABASIC       , ALEAVE       ,  3  );
    operation_add( &char_type[i++] , EndOfStatement ,               ACOMMA       , ALEAVE       ,  4  );
    operation_add( &char_type[i++] , CommaSeparator , RightAssoct | ACOMMA       , ALEAVE       ,  4  );
    operation_add( &char_type[i++] , Concatenate    ,               ABASIC       , ALEAVE       ,  5  );
    operation_add( &char_type[i++] , ConditionAsk   , RightAssoct | ABASIC       , ALEAVE       ,  6  );
    operation_add( &char_type[i++] , ConditionChoose, RightAssoct | ABASIC       , ALEAVE       ,  6  );
                                                                                          /* see 7,8,9 below */

    operation_add( &char_type[i++] , Assignment     ,               ACOMPARE     , ALEAVE       ,  12 );
    operation_add( &char_type[i++] , EqualTo        ,               ACOMPARE     , ALEAVE       ,  12 );
    operation_add( &char_type[i++] , SameAs         ,               ACOMPARE     , ALEAVE       ,  12 );
    operation_add( &char_type[i++] , NotSame        ,               ACOMPARE     , ALEAVE       ,  13 );
    operation_add( &char_type[i++] , NotEqual       ,               ACOMPARE     , ALEAVE       ,  13 );
    operation_add( &char_type[i++] , LessThan       ,               ACOMPARE     , ALEAVE       ,  14 );
    operation_add( &char_type[i++] , GreaterThan    ,               ACOMPARE     , ALEAVE       ,  14 );
    operation_add( &char_type[i++] , LessOrEqual    ,               ACOMPARE     , ALEAVE       ,  15 );
    operation_add( &char_type[i++] , GreaterOrEqual ,               ACOMPARE     , ALEAVE       ,  15 );

    operation_add( &char_type[i++] , Oper_add       ,               ABASIC       , ALEAVE       ,  16 );
    operation_add( &char_type[i++] , Oper_sub       ,               ABASIC       , ALEAVE       ,  17 );
    operation_add( &char_type[i++] , Oper_pos       , SkipClimbUp | ABASIC       , NOTLEAVE     ,  18 );
    operation_add( &char_type[i++] , Oper_neg       , SkipClimbUp | ABASIC       , NOTLEAVE     ,  19 );

    operation_add( &char_type[i++] , Oper_mul1      ,               ABASIC       , ALEAVE       ,  20 );
    operation_add( &char_type[i++] , Oper_mul2      ,               ABASIC       , ALEAVE       ,  20 );
    operation_add( &char_type[i++] , Oper_div1      ,               ABASIC       , ALEAVE       ,  20 );
    operation_add( &char_type[i++] , Oper_divI      ,               ABASIC       , ALEAVE       ,  20 );
    operation_add( &char_type[i++] , Oper_div2      ,               ABASIC       , ALEAVE       ,  20 );
    operation_add( &char_type[i++] , Oper_mul0      ,               ABASIC       , ALEAVE       ,  21 );
    operation_add( &char_type[i++] , Oper_pow1      , RightAssoct | ABASIC       , ALEAVE       ,  22 );
    operation_add( &char_type[i++] , Oper_pow2      , RightAssoct | ABASIC       , ALEAVE       ,  22 );
                                                                                            /* see 23 below */

    operation_add( &char_type[i++] , Bitwise_OR     ,               ABASIC       , ALEAVE       ,  24 );
    operation_add( &char_type[i++] , Bitwise_XOR    ,               ABASIC       , ALEAVE       ,  25 );
    operation_add( &char_type[i++] , Bitwise_AND    ,               ABASIC       , ALEAVE       ,  26 );
    operation_add( &char_type[i++] , Bitwise_NOT    , SkipClimbUp | ABASIC       , NOTLEAVE     ,  27 );
    operation_add( &char_type[i++] , Shift_Right    ,               ABASIC       , ALEAVE       ,  28 );
    operation_add( &char_type[i++] , Shift_Left     ,               ABASIC       , ALEAVE       ,  28 );

    operation_add( &char_type[i++] , Oper_dotproduct,               ABASIC       , ALEAVE       ,  30 );
    operation_add( &char_type[i++] , Oper_transpose ,               ACONSTANT    , ALEAVE       ,  31 );
    operation_add( &char_type[i++] , Oper_indexing  ,               ABASIC       , ALEAVE       ,  31 );

    operation_add( &char_type[i++] ,0,0,0,0);
}


static void operation_load_oper_type()
{
    int i=0;

    operation_add( &oper_type[i++] , Logical_OR      , RightAssoct | ALOGICAL     , ALEAVE      ,  7  );
    operation_add( &oper_type[i++] , Logical_AND     , RightAssoct | ALOGICAL     , ALEAVE      ,  8  );
    operation_add( &oper_type[i++] , Logical_NOT     , SkipClimbUp | ALOGICAL     , NOTLEAVE    ,  9  );

    operation_add( &oper_type[i++] , Oper_mod        , RightAssoct | ABASIC       , ALEAVE      ,  23 );

    operation_add( &oper_type[i++] ,0,0,0,0);
}


static void operation_load_word_type()
{
    int i=0;

    operation_add( &word_type[i++] , Constant_true      , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Constant_false     , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Constant_e_2_718_  , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Constant_pi_3_141_ , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , SQRT_of_Neg_One    , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_factorial , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_fullfloor , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_getprimes , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_srand     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_rand      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_gcd       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_ilog      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_isqrt     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_floor     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_ceil      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_sqrt      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_cbrt      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_exp       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_log       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_cos       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_sin       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_tan       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_acos      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_asin      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_atan      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_cosh      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_sinh      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_tanh      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_acosh     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_asinh     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_atanh     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_cabs      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_carg      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_real      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_imag      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_conj      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_proj      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_size      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_span      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_sum       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_max       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_min       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_vector    , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_range     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_try       , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_tostr     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_tonum     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_torat     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_toflt     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] , Function_eval      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_call      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_print     , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );
    operation_add( &word_type[i++] , Function_strlen    , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST );

    operation_add( &word_type[i++] ,0,0,0,0);
}


void operations_init (value stack)
{
    texts_load_twst(stack, RFET_ENGLISH);
    texts_load_twsf(stack, RFET_ENGLISH);
    operation_load_comp_type();
    operation_load_char_type();
    operation_load_oper_type();
    operation_load_word_type();
}


static value pointer_resolve (value v, const_value n, const_value beg, const_value end)
{
    if(end==NULL) end--;
    n = vGet(n);
    if(beg<=n && n<=end)
    {
        uint32_t a = *n;
        if((a>>28)==VAL_VECTOR)
        {
            a = VEC_LEN(a);
            value w = v;
            v += 2; // reserve space for vector header
            n += 2; // skip vector header
            int i;
            for(i=0; i<a; i++)
            {
                if(i) n = vNext(n);
                v = unOffset(pointer_resolve(v, n, beg, end));
            }
            setVector(w, a, v-w-2);
            v = setOffset(v, v-w);
        }
        else v = vcopy(v, n);
    }
    return v;
}


static value doCall (value v) // TODO: provide a special ValueType to store the container/component (same for 'this') which however prints itself as a string. But what if the container is inside a LHS and has been deleted? Answer: this will be equivalent to a constant value that has changed during parsing.
{
    value y = vPrev(v);
    const_value n = vGet(y);
    uint32_t a = *n;
    if((a>>28)==VAL_MESSAGE) return v;

    if((a>>28)==VAL_VECTOR){
        a = VEC_LEN(a);
        n += 2; // skip vector header
    } else a=1;

    int i, count;
    Container* container = NULL;
    {
        if(!a || !isStr2(n))
            return setError(y, L"1st argument must be the path to function to call.");
        bool fullAccess = n[1]&1;

        const wchar* wstr = getStr2(n);
        i = strlen2(wstr);
        lchar lstr[i+1];
        Str3 path = set_lchar_array(lstr, i+1, wstr, L"path to function");

        if(wstr && *wstr!='|')
            return setError(y, L"Given path to function must start with a '|'.");
        else container = container_find(v, NULL, path, fullAccess);
        if(!container)
        {
            assert(fullAccess == false);
            assert(VERROR(vnext(v)));
            return vcopy(y, v);
        }
    }
    n = vNext(n); // skip function name
    const_value t;
    uint64_t mask;
    if(a<=2) mask = count = a-1;
    else
    {   count = a-2;
        t = vGet(n);
        if(value_type(t) != aSmaInt)
            return setError(y, L"Second argument must be an integer mask.");
        mask = getSmaInt(t);
        n = vNext(n);
    }

    uint32_t arg[count*4];
    const_value m[count];
    int len[count];
    for(i=0; i<count; i++)
    {
        t = vGet(n);
        a = *t;
        if((a>>28)==VAL_VECTOR && (mask&(1L<<i))){
            a = VEC_LEN(a);
            if(a) { len[i] = a-1; m[i] = t+2; }
        } else a=0;
        if(!a) { len[i] = 0; m[i] = t; }
        n = vNext(n);
    }

    v += 2; // see e-=2 below
    value e = v;
    a = 0;
    while(true)
    {
        a++;
        value w = arg;
        for(i=0; i<count; i++)
            w = setAbsRef(w, m[i]);
        if(count>1) tovector(w, count);
        else if(count==0) setVector(arg,0,0);

        component_evaluate(v, container, container, arg);
        if(*v>>28) v = vNEXT(v);
        else return vcopy(y, v);

        w = NULL;
        for(i=0; i<count; i++)
            if(len[i]) {
                len[i]--;
                m[i] = vNext(m[i]);
                w++;
            }
        if(!w) break;
    }

    if(a!=1){ // if result is a vector
        e -= 2; // reserve space for vector header
        setVector(e, a, v-e-2); // set vector header
    }
    return vpcopy(y, pointer_resolve(v,e,y,NULL));
}


static value doEval (value v) // TODO: avoid nested eval(eval())
{
    value y = vPrev(v);
    const_value n = vGet(y);
    if((*n>>28)==VAL_MESSAGE) return v;

    const_value argument = NULL;
    uint32_t c = *n;
    if((c>>28)==VAL_VECTOR)
    {
        n += 2; // skip vector header
        c = VEC_LEN(c); // get vector length
        if(c==2) argument = vNext(n);
        else if(c!=1) n=NULL; // mark error
    }
    if(!n || !isStr2(n)) // if error
        v = setError(y, L"Valid usage is eval(\"rfet\") or\r\neval(\"\\\\(parameter)= rfet\", argument).");
    else
        v = vpcopy(y, rfet_parse_and_evaluate(v, getStr2(n), L"eval()", argument));
    return v;
}


static value valueSt_compare (value v, const_value obtained, const_value expected, value p)
{
    assert(obtained && expected);
    assert(vGet(expected)==expected);
    obtained = vGet(obtained);
    uint32_t a = *obtained;
    uint32_t b = *expected;

    if((b>>28)==VAL_VECTOR)     // if a vector is expected
    { if((a>>28)==VAL_VECTOR)   // if a vector is obtained
      {
        a = VEC_LEN(a);         // get vector length
        b = VEC_LEN(b);
        if(a != b) return NULL; // error, must have same length

        obtained += 2; // skip vector header
        expected += 2;
        while(a--)
        {
            p = valueSt_compare(v, obtained, expected, p);
            if(p==NULL) return NULL;
            obtained = vNext(obtained);
            expected = vNext(expected);
        }
        return p ? p : (value)1;
      }
      else return NULL;
    }
    else if((a>>28)==VAL_VECTOR
        &&  (!p || !VEC_LEN(a))) return NULL;

    if(p > (value)1) { p-=2; SetPtr(p, obtained); }
    return p ? p : (value)1;
}


static value compare_failed (value v, const_value obtained, const_value expected, int ID)
{
    const_Str2 argv[3];
    value w;
    v = VstToStr(setRef(v, obtained), PUT_CATEGORY|0, -1, -1);
    w = VstToStr(setRef(v, expected), PUT_CATEGORY|0, -1, -1);
    argv[0] = TWST(ID);
    argv[1] = getStr2(vGetPrev(v));
    argv[2] = getStr2(vGetPrev(w));
    return setMessage(w, 0, 3, argv);
}


value operations_evaluate (value stack, const_value oper)
{
    assert(oper && stack);
    if(!stack || !oper || !(*oper>>28))
        return vcopy(stack, oper); // TODO: review this line

    Component *comp = 0;
    const_value name = 0;
    const_Str2 argv[2];

    if((*oper>>28)==VAL_VECTOR) oper+=2;
    assert((*oper>>28)==VAL_OPERAT);
    value P = stack;
    value try = P;
    value v = P + OperEvalSize; // see expression.h
    v += P[0]>>16; // apply OperEval.start
    P[0] &= 0xFFFF;

    while(true) // main loop
    {
        uint32_t a = *oper;
        enum ID_TWSF ID = (a>>16)&0x0FFF;
        if(ID==0) // if end of evaluation then do a return-call
        {
            bool error = VERROR(v);
            if(a & 0xFFFF // if evaluating a component
            && !error)
            {
                // get the component that contains this expression
                memcpy(&comp, oper+1, sizeof(comp)); assert(comp!=NULL);

                // get to result of evaluation
                const_value r = vGetPrev(v);

                // if component has an expected result structure
                // then check result structure vs expected
                value e = c_expc(comp);
                if(*e && !valueSt_compare(v, r, e, NULL))
                {
                    // if comparison failed then get error message
                    v = compare_failed(v, r, e, ResultSt_vs_Expected);
                    error = true;
                }

                else
                if(comp->state==ISPARSE // if after component_finalise()
                && comp->para1[0]==0)   // and if component is a variable
                {
                    long size = vSize(r);
                    if(1+size < SIZEOF(comp->para1)*2)
                    {
                        // the if below is also found in component_destroy()
                        if(comp->constant != 1+comp->para1)
                            value_free(comp->constant);
                        comp->constant = 1+comp->para1;
                    }
                    else{ // code above is just malloc-phobia really!
                        comp->constant = value_alloc(comp->constant, size);
                        assert(comp->constant != NULL);
                    }
                    memcpy(comp->constant, r, size*sizeof(*r));
                    comp->instance = evaluation_instance(0);
                    comp->caller = (Container*)GetPtr(P+6);
                }
            }

            value p = P;
            oper = (const_value)GetPtr(p+4);
            try = p-p[3];
            P   = p-p[2];
            p   = (~p[1]==0) ? NULL : p-p[1]; // get final location of result

            if(!oper)
            {
                if(error)
                {
                    const_Str3 str; memcpy(&str, name, sizeof(str));
                    if(sChar(str)=='"' || sChar(str)=='\'')
                         argv[0] = L"Error on %s at (%s,%s) in %s:\r\n%s";
                    else argv[0] = L"Error on '%s' at (%s,%s) in %s:\r\n%s";
                    argv[1] = getMessage(vGetPrev(v));
                    v = setMessageE(v, 0, 2, argv, str);
                }
                if(p) v = vpcopy(p, v);
                break; // quit main loop
            }
            if(error) goto on_error;
            if(p) v = vpcopy(p, v);
            continue;
        }
        else name = oper+1; // record where the expression's name is located

        if(ID==OperJustJump) { oper += 1+(a & 0xFFFF); continue; }

        if(ID==SET_DOT_CALL     // if a container.variable call
        || ID==SET_DOT_FUNC)    // if a container.function call
        {
            v = vPrev(v);
            const_value n = vGet(v);
            if(!isStr2(n))
            {
                argv[0] = L"Left to operation must evaluate to a string.";
                v = setMessage(v, 0, 1, argv);
                goto on_error;
            }
            else
            {   bool fullAccess = n[1]&1; // see container_path_name()

                // construct pathname
                const_Str2 path = getStr2(n);
                long pLen = strlen2(path);
                lchar lstr[pLen+1];
                Str3 pathname = set_lchar_array(lstr, pLen+1, path, L"path");

                const_Str3 Name;
                memcpy(&Name, name, sizeof(Name));
                lstr[pLen].chr.c = '|';
                lstr[pLen].next = Name.ptr->next;
                pathname.end = Name.end;

                // get the component that contains this expression
                memcpy(&comp, oper+1+4, sizeof(comp)); assert(comp!=NULL);

                // get the containing container of that component
                Container* current = c_container(comp);

                Name.ptr = lstr; // get path as a Str3.
                Name.end = lstr+pLen;
                if(path && *path=='|' && !isSpecial3(sNext(Name))) // if path = "|filename"
                {
                    Container* container = container_find(v, current, Name, 0);
                    if(container==NULL) // if file was not loaded before
                    {
                        // Note: a 'pointing' string (i.e. for which Name.end != NULL) must
                        // be passed to container_parse() or else it will get memory-freed.
                        container = container_parse(v, NULL, Name, C37(NULL));
                        if(container && !dependence_parse(v)) container=NULL;
                        dependence_finalise(container!=NULL);
                        if(container==NULL) comp=NULL; // mark error
                    }
                }
                if(comp) comp = container_find(v, current, pathname, fullAccess);
                if(!comp) // if error
                {
                    assert(v == vPrev(vnext(v)));
                    v = vnext(v); // go to after VAL_MESSAGE
                    goto on_error;
                }
                int p = *c_para(comp); // get 1 + number of parameters
                if(p==0 && ID != SET_DOT_CALL) // if error
                {
                    argv[0] = L"Expected a variable not a function.";
                    v = setMessage(v, 0, 1, argv);
                    goto on_error;
                }
                if(p>=1 && ID != SET_DOT_FUNC) // if error
                {
                    argv[0] = L"Expected a function not a variable.";
                    v = setMessage(v, 0, 1, argv);
                    goto on_error;
                }
                ID = SET_DOT_CALL; // prepare for next if() below
            }
        }
        if(ID==SET_DOT_CALL
        || ID==SET_VAR_FUNC) // see struct OperEval inside expression.h
        {
            if(ID==SET_VAR_FUNC)
            {
                memcpy(&comp, oper+1+4, sizeof(comp)); assert(comp!=NULL);
                if(comp->replace         // if is a 'replaced' component
                || comp->state!=ISPARSE) // if before component_finalise()
                {
                    Container* caller = (Container*)GetPtr(P+6);
                    const_Str3 str; memcpy(&str, name, sizeof(str));
                    Component* c = component_find(v, caller, str, 0);
                    if(c && c_access(c)==ACCESS_REPLACE) comp = c;
                }

                // if variable already evaluated before
                if(comp->instance == evaluation_instance(0)
                && comp->caller == (Container*)GetPtr(P+6))
                {
                    v = vcopy(v, comp->constant); // then just copy it
                    oper += 1+(a & 0xFFFF);
                    continue;
                }
            }

            uint16_t recurs = P[0]>>16;
            if(++recurs == 10000) // if too many recursive calls
            {
                const_Str3 str; memcpy(&str, name, sizeof(str));
                setErrorE(v, L"Warning on '%s' at (%s,%s) in %s:\r\nFound too many recursive calls. Continue?", str);
                if(!wait_for_confirmation(L"Warning", getMessage(v)))
                {   v = setError(v, L"Found too many recursive calls.");
                    goto on_error;
                }
            }
            const_value w = c_para(comp);
            unsigned int C = *w;
            if(C)   // if a function then get number of parameters
               C--; // see component_insert() inside component.c
            assert(C<=0xFF);

            value p = v;
            if(C)
            {
                v = vPrev(v);   // get to function arguments
                p += 2*C;       // reserve memory for argument pointers
                w++;            // go to parameter value structure
                if(!valueSt_compare(p, v, w, p)) // set argument pointers
                {
                    v = compare_failed(p, v, w, Argument_vs_Parameter);
                    //p = NULL; // notify error for if(p) below
                    goto on_error;
                }
            }
            //if(p) // false if notified above
            {
                // push values to be recovered on return
                // see struct OperEval in expression.h
                p[0] = C | (P[0]&0xFF00) | ((uint32_t)recurs<<16);
                p[1] = p-v;
                p[2] = p-P;
                p[3] = p-try;
                oper += 1+(a & 0xFFFF);     // tell return-from-call to
                SetPtr(p+4, oper);          // point to the next operation

                if(ID==SET_DOT_CALL)                // if a container.component call
                     SetPtr(p+6, c_container(comp));// then get new caller container
                else SetPtr(p+6, GetPtr(P+6));      // else copy old caller

                // finally do the component call
                assert(c_oper(comp)!=NULL);
                oper = c_oper(comp);
                if((*oper>>28)==VAL_VECTOR) oper+=2;
                assert((*oper>>28)==VAL_OPERAT);
                P = p;
                try = P;
                v = P + OperEvalSize; // see expression.h
                continue;
            }
        }
        else if(ID==SET_PARAMTER)
        {
            int i = oper[1];
            assert(i < (P[0]&0xFF));
            const_value n = (const_value)GetPtr(P-(i+1)*2);
            v = vcopy(v, n);
            oper += 1+(a & 0xFFFF);
            continue;
        }
        else if(ID==ConditionAsk)
        {
            v = vPrev(_not(v));
            const_value n = vGet(v);
            if(!isBool(n)) // if result is not boolean
            {
                assert((*n>>28)==VAL_VECTOR);
                argv[0] = TWST(Condition_IsNot_Single);
                v = setMessage(v, 0, 1, argv);
            }
            else if(*n & 1) // if condition was false: _not(false) == true
                { oper += *(oper+1+4); continue; }
            else{ oper += 1+(a & 0xFFFF); continue; }
        }
        else if(ID==Function_try)
        {
            if(try==P) { try=v; oper += 1+(a & 0xFFFF); }
            else
            {
                name = oper+1;
                argv[0] = L"A try() inside another is not allowed.";
                v = setMessage(v, 0, 1, argv);
                while(true) // jump to the end of opers
                {   oper += 1+(a & 0xFFFF);
                    a = *oper;
                    if(((a>>16)&0x0FFF)==0) break;
                }
            }
            continue;
        }
        else if(ID==Function_try_that)
        {
            while(true) // jump to the corresponding catch
            {   a = *oper;
                oper += 1+(a & 0xFFFF);
                if(((a>>16)&0x0FFF)==Function_try_catch) { try=P; break; }
            }
            continue;
        }
        else if(ID==Replacement) // Also see expression_to_operation() in expression.c
        {
            long size = oper[1+4+2+2];  // get offset to the LHS constant
            value w = (value)(size_t)(oper - size);   // get LHS constant

            if((*(w-1) & 0xFFFF)==0)    // if LHS code was just executed just now
            {
                v = vPrev(v);
                const_value n = vGet(v);
                *(w-1) = (VAL_OPERAT<<28) | (OperJustJump<<16) | (uint16_t)size;
                memcpy(w, n, vSize(n)*sizeof(*n)); // TODO: check size limit, also use vcopy instead
            }
            value t = (value)(size_t)(oper+1+4+2);
            size = evaluation_instance(0);
            if(0!=memcmp(t, &size, sizeof(size)))
            {
                memcpy(t, &size, sizeof(size));

                memcpy(&comp, oper+1+4, sizeof(comp));
                if(c_container(comp) == (Container*)GetPtr(P+6)) // check if on caller container
                {
                    // go execute the Right Hand Size of ':='
                    oper += 1+(a & 0xFFFF);
                    continue;
                }
            }
            v = vcopy(v, w); // copy LHS constant of ':='
            oper += oper[1+4+2+2+1]; // skip RHS code of ':='
            continue;
        }
        else if(ID==ReplaceRecord)
        {
            const_value repl = oper - oper[1];
            value w = (value)(size_t)(repl - repl[1+4+2+2]); // get LHS constant
            const_value u = vGetPrev(v);

            sameAs(setRef(setRef(v, w), u));
            if(!(*v & 1)) // if w and u are not equal
            {
                memcpy(w, u, (v-u-1)*sizeof(*u)); // copy new LHS constant
                replacement_record(repl);
            }
            oper += 1+(a & 0xFFFF);
            continue;
        }
        else if(ID==Function_print)
        {
            v = VstToStr(v, PUT_NEWLINE|0, -1, -1);
            argv[0] = L"[On \"%s\" at (%s,%s) in %s]\r\n%s";
            argv[1] = getStr2(vGetPrev(v));
            const_Str3 str; memcpy(&str, name, sizeof(str));
            v = vpcopy(stack, setMessageE(v, 0, 2, argv, str));
            break; // quit main loop
        }
        else
        { switch(ID)
          {
            case Function_try_catch: try=P; break;
            case SET_CONSTANT: v = vcopy(v, oper+1); break;
            case SET_REPL_LHS: v = vcopy(v, oper - oper[1]); break;
            case SET_OUTSIDER: v = set_outsider(v, oper[1+4]); break;

            case Logical_OR     : v = logical_or(v); break;
            case Logical_AND    : v = logical_and(v); break;
            case Logical_NOT    : v = logical_not(v); break;

            case Assignment     : v = equalTo(v); break;
            case EqualTo        : v = equalTo(v); break;
            case SameAs         : v = sameAs(v); break;
            case NotSame        : v = notSame(v); break;
            case NotEqual       : v = notEqual(v); break;
            case LessThan       : v = lessThan(v); break;
            case GreaterThan    : v = greaterThan(v); break;
            case LessOrEqual    : v = lessOrEqual(v); break;
            case GreaterOrEqual : v = greaterOrEqual(v); break;

            case Oper_pos       : v = _pos(v); break;
            case Oper_neg       : v = _neg(v); break;
            case Oper_add       : v = _add(v); break;
            case Oper_sub       : v = _sub(v); break;

            case Oper_mul0      :
            case Oper_mul1      : v = __mul(v); break;
            case Oper_mul2      : v = _mul(v); break;
            case Oper_div1      : v = __div(v); break;
            case Oper_divI      : v = _idiv(v); break;
            case Oper_div2      : v = _div(v); break;
            case Oper_pow1      : v = power(v); break;
            case Oper_pow2      : v = _pow(v); break;
            case Oper_mod       : v = _mod(v); break;

            case Bitwise_OR     : v = bitwise_or(v); break;
            case Bitwise_XOR    : v = bitwise_xor(v); break;
            case Bitwise_AND    : v = bitwise_and(v); break;
            case Bitwise_NOT    : v = bitwise_not(v); break;
            case Shift_Right    : v = shift_right(v); break;
            case Shift_Left     : v = shift_left(v); break;

            case CommaSeparator : v = tovector(v, oper[1]); break;
            case Concatenate    : v = combine(v); break;
            case Oper_indexing  : v = indexing(v); break;
            case Oper_transpose : v = transpose(v); break;
            case Oper_dotproduct: v = dotproduct(v); break;

            case Constant_true      : v = setBool(v, true); break;
            case Constant_false     : v = setBool(v, false); break;
            case Constant_e_2_718_  : v = _exp (setSmaInt(v, 1)); break;
            case Constant_pi_3_141_ : v = _acos(setSmaInt(v,-1)); break;
            case SQRT_of_Neg_One    : v = _sqrt(setSmaInt(v,-1)); break;

            case Function_factorial : v = factorial(v); break;
            case Function_fullfloor : v = fullfloor(v); break;
            case Function_getprimes : v = getprimes(v); break;
            case Function_srand     : v = _srand(v); break;
            case Function_rand      : v = _rand(v); break;

            case Function_gcd   : v = _gcd(v); break;
            case Function_ilog  : v = _ilog(v); break;
            case Function_isqrt : v = _isqrt(v); break;
            case Function_floor : v = _floor(v); break;
            case Function_ceil  : v = _ceil(v); break;

            case Function_sqrt  : v = _sqrt(v); break;
            case Function_cbrt  : v = _cbrt(v); break;
            case Function_exp   : v = _exp(v); break;
            case Function_log   : v = _log(v); break;

            case Function_cos   : v = _cos(v); break;
            case Function_sin   : v = _sin(v); break;
            case Function_tan   : v = _tan(v); break;
            case Function_acos  : v = _acos(v); break;
            case Function_asin  : v = _asin(v); break;
            case Function_atan  : v = _atan(v); break;

            case Function_cosh  : v = _cosh(v); break;
            case Function_sinh  : v = _sinh(v); break;
            case Function_tanh  : v = _tanh(v); break;
            case Function_acosh : v = _acosh(v); break;
            case Function_asinh : v = _asinh(v); break;
            case Function_atanh : v = _atanh(v); break;

            case Function_cabs  : v = _cabs(v); break;
            case Function_carg  : v = _carg(v); break;
            case Function_real  : v = _real(v); break;
            case Function_imag  : v = _imag(v); break;
            case Function_conj  : v = _conj(v); break;
            case Function_proj  : v = _proj(v); break;

            case Function_size  : v = _size(v); break;
            case Function_span  : v = _span(v); break;
            case Function_sum   : v = _sum(v); break;
            case Function_max   : v = _max(v); break;
            case Function_min   : v = _min(v); break;

            case Function_vector: v = _vector(v); break;
            case Function_range : v = _range(v); break;

            case Function_tostr : v = toStr(v); break;
            case Function_tonum : v = toNum(v); break;
            case Function_torat : v = toRat(v); break;
            case Function_toflt : v = toFlt(v); break;

            case Function_eval  : v = doEval(v); break;
            case Function_call  : v = doCall(v); break;
            case Function_strlen: v = vStrLen(v); break;

            default: assert(false); break;
          }
          oper += 1+(a & 0xFFFF);
        }
        if(VERROR(v))
        {
            on_error: ;
            bool f=1;
            while(true)
            {
                a = *oper;
                ID = ((a>>16)&0x0FFF);
                if(ID==0) break;
                oper += 1+(a & 0xFFFF);
                if(ID==Function_try_that  && f) { v=try; break; }
                if(ID==Function_try_catch && f) f=0;
            }
        }
    }
    return v;
}

