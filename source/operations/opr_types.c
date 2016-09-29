/*
    opr_types.c

    This file is #included by operations.c only.
*/

#define OPERANDS_DONOT_MATCH set_error_message (TWST(Operands_DoNot_Match), name, TIS2(0,headSize), TIS2(1,lastSize));

#define CHECK_Y if(!y.type) { eval_error(name); value_free(output); output=NULL; break; }

static void eval_error (const lchar* name)
{
    strcpy22(error_message+500, error_message);
    set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4':\r\n\\5"), name, error_message+500);
}

//--------------------------------------------------------------------------------------------------------------------------------------

static Value* OPER1 (Expression* expression, const Value* argument,
                     Value* OPR (const Value* in,
                                 const lchar* name,
                                 int info,
                                 Value CALL (Value)),
                     int info,
                     Value CALL (Value))
{
    Expression *head = expression->headChild;
    Value* in = expression_evaluate(head, argument);
    Value* out = OPR (in, expression->name, info, CALL);

    if(0/*first time*/ && out)
    {   expression->independent = head->independent;
        INDEPENDENT(expression)
    }
    value_free(in);
    return out;
}

static Value* OPER5 (Expression* expression, const Value* argument,
                     Value* OPR (const Value* in,
                                 const lchar* name,
                                 int info,
                                 Value CALL (Value, Value)),
                     int info,
                     Value CALL (Value, Value))
{
    Expression *head = expression->headChild;
    Value* in = expression_evaluate(head, argument);
    Value* out = OPR (in, expression->name, info, CALL);

    if(0/*first time*/ && out)
    {   expression->independent = head->independent;
        INDEPENDENT(expression)
    }
    value_free(in);
    return out;
}

static Value* OPER2 (Expression* expression, const Value* argument,
                     Value* OPR (const Value* headVst,
                                 const Value* lastVst,
                                 const lchar* name,
                                 int info,
                                 Value CALL (Value, Value)),
                     int info,
                     Value CALL (Value, Value))
{
    Value *out=NULL, *headVst=NULL, *lastVst=NULL;
    Expression* head = expression->headChild;
    Expression* last = expression->lastChild;

    if((headVst = expression_evaluate(head, argument))
    && (lastVst = expression_evaluate(last, argument)))
        out = OPR (headVst, lastVst, expression->name, info, CALL);

    if(0/*first time*/ && out)
    {   expression->independent = MIND(head->independent, last->independent);
        INDEPENDENT(expression)
    }
    value_free(headVst);
    value_free(lastVst);
    return out;
}

static Value* OPER4 (Expression* expression, const Value* argument,
                     Value* OPR (const Value* headVst,
                                 const Value* lastVst,
                                 const lchar* name,
                                 int info,
                                 Value CALL (Value, Value, Value)),
                     int info,
                     Value CALL (Value, Value, Value))
{
    Value *out=NULL, *headVst=NULL, *lastVst=NULL;
    Expression* head = expression->headChild;
    Expression* last = expression->lastChild;

    if((headVst = expression_evaluate(head, argument))
    && (lastVst = expression_evaluate(last, argument)))
        out = OPR (headVst, lastVst, expression->name, info, CALL);

    if(0/*first time*/ && out)
    {   expression->independent = MIND(head->independent, last->independent);
        INDEPENDENT(expression)
    }
    value_free(headVst);
    value_free(lastVst);
    return out;
}



//--------------------------------- TYPE 1 OPERATION : 1 argument ------------------------------

Value* TYPE1 (const Value* in, const lchar* name, int info, Value CALL (Value))
{
    if(in==NULL) return NULL;
    Value y, n;
    Value *out, *output;
    long size = VST_LEN(in);
    output = out = value_alloc(size);

    for( ; size--; in++)
    {
        n = *in;
        if(!isSeptor(n))
        { y = CALL (n); CHECK_Y } else y=n;
        *out++ = y;
    }
    return output;
}



//--------------------------------- TYPE 2 OPERATION : 2 arguments per-value operation --------------

Value* TYPE2 (const Value* headVst, const Value* lastVst, const lchar* name, int info, Value CALL (Value, Value))
{
    if(!headVst || !lastVst) return NULL;
    Value y, n, m;
    Value *out, *output;
    long headSize = VST_LEN(headVst);
    long lastSize = VST_LEN(lastVst);

    if(headSize==1 && lastSize==1)
    {
        output = out = value_alloc(1);
        n = *headVst;
        m = *lastVst;
        for(;;)
        {
            { y = CALL (n, m); CHECK_Y }
            *out = y;
            break;
        }
    }
    else if(info)
    {
        if(headSize!=1) set_error_message (TWST(Left_IsNot_Single ), name, TIS2(0,headSize));
        if(lastSize!=1) set_error_message (TWST(Right_IsNot_Single), name, TIS2(0,lastSize));
        output=NULL;
    }
    else if(lastSize==1)
    {
        output = out = value_alloc(headSize);
        m = *lastVst;
        for( ; headSize--; headVst++)
        {
            n = *headVst;
            if(!isSeptor(n))
            { y = CALL (n, m); CHECK_Y } else y=n;
            *out++ = y;
        }
    }
    else if(headSize==1)
    {
        output = out = value_alloc(lastSize);
        n = *headVst;
        for( ; lastSize--; lastVst++)
        {
            m = *lastVst;
            if(!isSeptor(m))
            { y = CALL (n, m); CHECK_Y } else y=m;
            *out++ = y;
        }
    }
    else if(0==valueSt_compare(headVst, lastVst))
    {
        output = out = value_alloc(headSize);
        for( ; headSize--; headVst++, lastVst++)
        {
            n = *headVst;
            m = *lastVst;
            if(!isSeptor(n))
            { y = CALL (n, m); CHECK_Y } else y=n;
            *out++ = y;
        }
    }
    else { OPERANDS_DONOT_MATCH output=NULL; }
    return output;
}



//--------------------------------- TYPE 4 OPERATION : 2 arguments, 1 result ------------------------------

Value* TYPE4 (const Value* headVst, const Value* lastVst, const lchar* name, int info, Value CALL (Value, Value, Value))
{
    if(!headVst || !lastVst) return NULL;
    Value n, m, y; y.type=0;
    Value *output = value_alloc(1);
    long headSize = VST_LEN(headVst);
    long lastSize = VST_LEN(lastVst);

    if(lastSize==1)
    {
        m = *lastVst;
        for( ; headSize--; headVst++)
        {
            n = *headVst;
            if(!isSeptor(n))
            { y = CALL (y, n, m); CHECK_Y }
        }
    }
    else if(headSize==1)
    {
        n = *headVst;
        for( ; lastSize--; lastVst++)
        {
            m = *lastVst;
            if(!isSeptor(m))
            { y = CALL (y, n, m); CHECK_Y }
        }
    }
    else if(0==valueSt_compare(headVst, lastVst))
    {
        for( ; headSize--; headVst++, lastVst++)
        {
            n = *headVst;
            m = *lastVst;
            if(!isSeptor(n))
            { y = CALL (y, n, m); CHECK_Y }
        }
    }
    else { OPERANDS_DONOT_MATCH value_free(output); output=NULL; }
    if(output) *output=y;
    return output;
}



//--------------------------------- TYPE 5 OPERATION : 1 argument, 1 result ------------------------------

Value* TYPE5 (const Value* in, const lchar* name, int info, Value CALL (Value, Value))
{
    if(in==NULL) return NULL;
    long len = VST_LEN(in);
    Value y; y.type=0;
    Value* output=NULL;

    for( ; len; len--, in++)
    {
        Value n = *in;
        if(!isSeptor(n))
        { y = CALL(y,n); CHECK_Y }
    }
    if(len==0)
    {   output = value_alloc(1);
        *output = y;
    }
    return output;
}



//--------------------------------------------------------------------------------------------------------------------------------------

Value* convert_to_sma          (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, toSma); }
Value* convert_to_big          (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, toBig); }

Value* fct_exp_e               (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _exp_e); }
Value* fct_log_e               (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _log_e); }
Value* fct_log_10              (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _log_10); }
Value* fct_sqrt                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _sqrt); }
Value* fct_ceil                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _ceil); }
Value* fct_floor               (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _floor); }

Value* fct_abs                 (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _absolute); }
Value* fct_arg                 (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _argument); }
Value* fct_real                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _real_part); }
Value* fct_imag                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _imag_part); }
Value* fct_conj                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _conjugate); }
Value* fct_proj                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _projection); }

Value* fct_cos                 (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _cos); }
Value* fct_sin                 (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _sin); }
Value* fct_tan                 (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _tan); }
Value* fct_acos                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _acos); }
Value* fct_asin                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _asin); }
Value* fct_atan                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _atan); }

Value* fct_cosh                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _cosh); }
Value* fct_sinh                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _sinh); }
Value* fct_tanh                (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _tanh); }
Value* fct_acosh               (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _acosh); }
Value* fct_asinh               (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _asinh); }
Value* fct_atanh               (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, _atanh); }

Value* opr_positive            (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, positive); }
Value* opr_negative            (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, negative); }
Value* opr_bitwise_not         (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, bitwise_not); }
Value* opr_logical_not         (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, logical_not); }
Value* opr_factorial           (Expression* expression, const Value* argument) { return OPER1 (expression, argument, TYPE1, 0, factorial); }

//--------------------------------------------------------------------------------------------------------------------------------------

Value* opr_plus_type2          (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, add); }
Value* opr_minus_type2         (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, subtract); }
Value* opr_times_type2         (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, multiply); }
Value* opr_divide_type2        (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, divide); }
Value* opr_modulo_type2        (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, modulo); }
Value* opr_topower_type2       (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, power); }

Value* opr_shift_right_type2   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, shift_right); }
Value* opr_shift_left_type2    (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, shift_left); }
Value* opr_bitwise_or_type2    (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, bitwise_or); }
Value* opr_bitwise_xor_type2   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, bitwise_xor); }
Value* opr_bitwise_and_type2   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, bitwise_and); }
Value* opr_logical_or_type2    (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, logical_or); }
Value* opr_logical_and_type2   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, logical_and); }

Value* opr_equal_type2         (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, equalTo); }
Value* opr_notEqual_type2      (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, notEqual); }
Value* opr_lessThan_type2      (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, lessThan); }
Value* opr_greaterThan_type2   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, greaterThan); }
Value* opr_lessOrEqual_type2   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, lessOrEqual); }
Value* opr_greaterOrEqual_type2(Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 0, greaterOrEqual); }

Value* opr_divide_type3        (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 1, divide); }
Value* opr_topower_type3       (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 1, power); }
Value* opr_lessThan_type3      (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 1, lessThan); }
Value* opr_greaterThan_type3   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 1, greaterThan); }
Value* opr_lessOrEqual_type3   (Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 1, lessOrEqual); }
Value* opr_greaterOrEqual_type3(Expression* expression, const Value* argument) { return OPER2 (expression, argument, TYPE2, 1, greaterOrEqual); }

//--------------------------------------------------------------------------------------------------------------------------------------

Value dot_product (Value y, Value n, Value m)
{
    if(!y.type) setSmaInt(y, 0);
    return add(y, multiply(n,m));
}

Value do_equalTo (Value y, Value n, Value m)
{
    if(!y.type) setSmaInt(y, true);
    Value t = equalTo(n,m);
    if(!t.type) y.type=0;
    else if(!getSmaInt(t)) setSmaInt(y,false);
    return y;
}

Value do_notEqual (Value y, Value n, Value m)
{
    if(!y.type) setSmaInt(y, true);
    Value t = notEqual(n,m);
    if(!t.type) y.type=0;
    else if(!getSmaInt(t)) setSmaInt(y,false);
    return y;
}

Value* opr_dot_product    (Expression* expression, const Value* argument) { return OPER4 (expression, argument, TYPE4, 0, dot_product); }
Value* opr_equal_type4    (Expression* expression, const Value* argument) { return OPER4 (expression, argument, TYPE4, 0, do_equalTo ); }
Value* opr_notEqual_type4 (Expression* expression, const Value* argument) { return OPER4 (expression, argument, TYPE4, 0, do_notEqual); }

//--------------------------------------------------------------------------------------------------------------------------------------

Value get_len (Value y, Value n) { setSmaInt(n,1); if(!y.type) setSmaInt(y,0); return add(y,n); }
Value get_sum (Value y, Value n) {                 if(!y.type) setSmaInt(y,0); return add(y,n); }
Value get_max (Value y, Value n) { return (!y.type || getSmaInt(lessThan(y,n))) ? n : y; }
Value get_min (Value y, Value n) { return (!y.type || getSmaInt(lessThan(n,y))) ? n : y; }

Value* fct_len (Expression* expression, const Value* argument) { return OPER5 (expression, argument, TYPE5, 0, get_len); }
Value* fct_sum (Expression* expression, const Value* argument) { return OPER5 (expression, argument, TYPE5, 0, get_sum); }
Value* fct_max (Expression* expression, const Value* argument) { return OPER5 (expression, argument, TYPE5, 0, get_max); }
Value* fct_min (Expression* expression, const Value* argument) { return OPER5 (expression, argument, TYPE5, 0, get_min); }

