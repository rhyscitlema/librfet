/*
    opr_types.c

    This file is #included by operations.c only.
*/

#define OPERANDS_DONOT_MATCH { set_message(eca.garg->message, TWST(Operands_DoNot_Match), expression->name, TIS2(0,headLen), TIS2(1,lastLen)); return 0; }

#define CHECK_Y if(!getType(y)) { eval_error(eca.garg, y, expression->name); return 0; }

static void eval_error (GeneralArg* garg, value y, const lchar* name)
{ set_message(garg->message, L"Error in \\1 at \\2:\\3 on '\\4':\r\n\\5", name, getNotval(y)); }



//--------------------------------- TYPE 1 OPERATION : 1 argument per-value-operation ----------------

bool TYPE1 (ExprCallArg eca, value CALL1 (value))
{
    value y;
    long headLen;
    value* headVst;

    Expression* expression = eca.expression;
    Expression* head = expression->headChild;

    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;
    headVst = eca.stack;
    headLen = VST_LEN(headVst);

    for( ; headLen--; headVst++)
    {
        y = *headVst;
        if(!isSeptor(y))
        {   y = CALL1(y); CHECK_Y
            *headVst = y;
        }
    }
    if(expression->independent==0) // if first time
        INDEPENDENT(expression, eca.stack, head->independent);
    return 1;
}



//--------------------------------- TYPE 5 OPERATION : 1 argument 1 result ------------------------------

bool TYPE4 (ExprCallArg eca, value CALL2 (value, value))
{
    value n, y={0};
    long headLen;
    value* headVst;

    Expression* expression = eca.expression;
    Expression* head = expression->headChild;

    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;
    headVst = eca.stack;
    headLen = VST_LEN(headVst);

    for( ; headLen--; headVst++)
    {
        n = *headVst;
        if(!isSeptor(n))
        { y = CALL2(y,n); CHECK_Y }
    }
    *eca.stack = y;
    if(expression->independent==0) // if first time
        INDEPENDENT(expression, eca.stack, head->independent);
    return 1;
}



//--------------------------------- TYPE 2 OPERATION : 2 arguments per-value operation --------------

bool TYPE2 (ExprCallArg eca, bool type5, value CALL2 (value, value))
{
    value y, n, m;
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

    if(lastLen==1)
    {
      m = *lastVst;
      if(headLen==1)
      {
        for(;;)
        {
            y = *headVst;
            { y = CALL2 (y, m); CHECK_Y }
            *headVst = y;
            break;
        }
      }
      else if(type5)
      {
        if(headLen!=1) set_message(eca.garg->message, TWST(Left_IsNot_Single ), expression->name, TIS2(0,headLen));
        if(lastLen!=1) set_message(eca.garg->message, TWST(Right_IsNot_Single), expression->name, TIS2(0,lastLen));
        return 0;
      }
      else
      {
        for( ; headLen--; headVst++)
        {
            y = *headVst;
            if(!isSeptor(y))
            { y = CALL2 (y, m); CHECK_Y }
            *headVst = y;
        }
      }
    }
    else if(headLen==1)
    {
        n = *headVst;
        for( ; lastLen--; headVst++, lastVst++)
        {
            y = *lastVst;
            if(!isSeptor(y))
            { y = CALL2 (n, y); CHECK_Y }
            *headVst = y;
        }
    }
    else if(0==valueSt_compare(headVst, lastVst))
    {
        for( ; headLen--; headVst++, lastVst++)
        {
            y = *headVst;
            m = *lastVst;
            if(!isSeptor(y))
            { y = CALL2 (y, m); CHECK_Y }
            *headVst = y;
        }
    }
    else OPERANDS_DONOT_MATCH

    if(expression->independent==0) // if first time
        INDEPENDENT(expression, stack, MIND(head->independent, last->independent));
    return 1;
}



//--------------------------------- TYPE 4 OPERATION : 2 arguments, 1 result ------------------------------

bool TYPE3 (ExprCallArg eca, value CALL3 (value, value, value))
{
    value n, m, y={0};
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

    if(lastLen==1)
    {
        m = *lastVst;
        for( ; headLen--; headVst++)
        {
            n = *headVst;
            if(!isSeptor(n))
            { y = CALL3 (y, n, m); CHECK_Y }
        }
    }
    else if(headLen==1)
    {
        n = *headVst;
        for( ; lastLen--; lastVst++)
        {
            m = *lastVst;
            if(!isSeptor(m))
            { y = CALL3 (y, n, m); CHECK_Y }
        }
    }
    else if(0==valueSt_compare(headVst, lastVst))
    {
        for( ; headLen--; headVst++, lastVst++)
        {
            n = *headVst;
            m = *lastVst;
            if(!isSeptor(n))
            { y = CALL3 (y, n, m); CHECK_Y }
        }
    }
    else OPERANDS_DONOT_MATCH

    *stack = y;
    if(expression->independent==0) // if first time
        INDEPENDENT(expression, stack, MIND(head->independent, last->independent));
    return 1;
}



//--------------------------------------------------------------------------------------------------------------------------------------

bool convert_to_rat          (ExprCallArg eca) { return TYPE1 (eca, toRat); }
bool convert_to_flt          (ExprCallArg eca) { return TYPE1 (eca, toFlt); }

bool fct_exp_e               (ExprCallArg eca) { return TYPE1 (eca, _exp_e); }
bool fct_log_e               (ExprCallArg eca) { return TYPE1 (eca, _log_e); }
bool fct_log_10              (ExprCallArg eca) { return TYPE1 (eca, _log_10); }
bool fct_sqrt                (ExprCallArg eca) { return TYPE1 (eca, _sqrt); }
bool fct_ceil                (ExprCallArg eca) { return TYPE1 (eca, _ceil); }
bool fct_floor               (ExprCallArg eca) { return TYPE1 (eca, _floor); }

bool fct_cabs                (ExprCallArg eca) { return TYPE1 (eca, __abs); }
bool fct_carg                (ExprCallArg eca) { return TYPE1 (eca, _carg); }
bool fct_real                (ExprCallArg eca) { return TYPE1 (eca, _real); }
bool fct_imag                (ExprCallArg eca) { return TYPE1 (eca, _imag); }
bool fct_conj                (ExprCallArg eca) { return TYPE1 (eca, _conj); }
bool fct_proj                (ExprCallArg eca) { return TYPE1 (eca, _proj); }

bool fct_cos                 (ExprCallArg eca) { return TYPE1 (eca, _cos); }
bool fct_sin                 (ExprCallArg eca) { return TYPE1 (eca, _sin); }
bool fct_tan                 (ExprCallArg eca) { return TYPE1 (eca, _tan); }
bool fct_acos                (ExprCallArg eca) { return TYPE1 (eca, _acos); }
bool fct_asin                (ExprCallArg eca) { return TYPE1 (eca, _asin); }
bool fct_atan                (ExprCallArg eca) { return TYPE1 (eca, _atan); }

bool fct_cosh                (ExprCallArg eca) { return TYPE1 (eca, _cosh); }
bool fct_sinh                (ExprCallArg eca) { return TYPE1 (eca, _sinh); }
bool fct_tanh                (ExprCallArg eca) { return TYPE1 (eca, _tanh); }
bool fct_acosh               (ExprCallArg eca) { return TYPE1 (eca, _acosh); }
bool fct_asinh               (ExprCallArg eca) { return TYPE1 (eca, _asinh); }
bool fct_atanh               (ExprCallArg eca) { return TYPE1 (eca, _atanh); }

bool opr_positive            (ExprCallArg eca) { return TYPE1 (eca, positive); }
bool opr_negative            (ExprCallArg eca) { return TYPE1 (eca, negative); }
bool opr_bitwise_not         (ExprCallArg eca) { return TYPE1 (eca, bitwise_not); }
bool opr_logical_not         (ExprCallArg eca) { return TYPE1 (eca, logical_not); }
bool opr_factorial           (ExprCallArg eca) { return TYPE1 (eca, factorial); }

//--------------------------------------------------------------------------------------------------------------------------------------

bool opr_plus_type2          (ExprCallArg eca) { return TYPE2 (eca, 0, add); }
bool opr_minus_type2         (ExprCallArg eca) { return TYPE2 (eca, 0, subtract); }
bool opr_times_type2         (ExprCallArg eca) { return TYPE2 (eca, 0, multiply); }
bool opr_divide_type2        (ExprCallArg eca) { return TYPE2 (eca, 0, divide); }
bool opr_modulo_type2        (ExprCallArg eca) { return TYPE2 (eca, 0, modulo); }
bool opr_topower_type2       (ExprCallArg eca) { return TYPE2 (eca, 0, power); }

bool opr_shift_right_type2   (ExprCallArg eca) { return TYPE2 (eca, 0, shift_right); }
bool opr_shift_left_type2    (ExprCallArg eca) { return TYPE2 (eca, 0, shift_left); }
bool opr_bitwise_or_type2    (ExprCallArg eca) { return TYPE2 (eca, 0, bitwise_or); }
bool opr_bitwise_xor_type2   (ExprCallArg eca) { return TYPE2 (eca, 0, bitwise_xor); }
bool opr_bitwise_and_type2   (ExprCallArg eca) { return TYPE2 (eca, 0, bitwise_and); }
bool opr_logical_or_type2    (ExprCallArg eca) { return TYPE2 (eca, 0, logical_or); }
bool opr_logical_and_type2   (ExprCallArg eca) { return TYPE2 (eca, 0, logical_and); }

bool opr_equal_type2         (ExprCallArg eca) { return TYPE2 (eca, 0, equalTo); }
bool opr_notEqual_type2      (ExprCallArg eca) { return TYPE2 (eca, 0, notEqual); }
bool opr_lessThan_type2      (ExprCallArg eca) { return TYPE2 (eca, 0, lessThan); }
bool opr_greaterThan_type2   (ExprCallArg eca) { return TYPE2 (eca, 0, greaterThan); }
bool opr_lessOrEqual_type2   (ExprCallArg eca) { return TYPE2 (eca, 0, lessOrEqual); }
bool opr_greaterOrEqual_type2(ExprCallArg eca) { return TYPE2 (eca, 0, greaterOrEqual); }

bool opr_divide_type5        (ExprCallArg eca) { return TYPE2 (eca, 1, divide); }
bool opr_topower_type5       (ExprCallArg eca) { return TYPE2 (eca, 1, power); }
bool opr_lessThan_type5      (ExprCallArg eca) { return TYPE2 (eca, 1, lessThan); }
bool opr_greaterThan_type5   (ExprCallArg eca) { return TYPE2 (eca, 1, greaterThan); }
bool opr_lessOrEqual_type5   (ExprCallArg eca) { return TYPE2 (eca, 1, lessOrEqual); }
bool opr_greaterOrEqual_type5(ExprCallArg eca) { return TYPE2 (eca, 1, greaterOrEqual); }

//--------------------------------------------------------------------------------------------------------------------------------------

value dot_product (value y, value n, value m)
{
    if(!getType(y)) y = setSmaInt(0);
    return add(y, multiply(n,m));
}

value do_equalTo (value y, value n, value m)
{
    if(!getType(y)) y = setSmaInt(true);
    value t = equalTo(n,m);
    if(!getType(t)) y = setNotval(0);
    else if(!getSmaInt(t)) y = setSmaInt(false);
    return y;
}

value do_notEqual (value y, value n, value m)
{
    if(!getType(y)) y = setSmaInt(true);
    value t = notEqual(n,m);
    if(!getType(t)) y = setNotval(0);
    else if(!getSmaInt(t)) y = setSmaInt(false);
    return y;
}

bool opr_dot_product    (ExprCallArg eca) { return TYPE3 (eca, dot_product); }
bool opr_equal_type4    (ExprCallArg eca) { return TYPE3 (eca, do_equalTo ); }
bool opr_notEqual_type4 (ExprCallArg eca) { return TYPE3 (eca, do_notEqual); }

//--------------------------------------------------------------------------------------------------------------------------------------

value get_len (value y, value n) { n=setSmaInt(1); if(!getType(y)) y=setSmaInt(0); return add(y,n); }
value get_sum (value y, value n) {                 if(!getType(y)) y=setSmaInt(0); return add(y,n); }
value get_max (value y, value n) { return (!getType(y) || getSmaInt(lessThan(y,n))) ? n : y; }
value get_min (value y, value n) { return (!getType(y) || getSmaInt(lessThan(n,y))) ? n : y; }

bool fct_len (ExprCallArg eca) { return TYPE4 (eca, get_len); }
bool fct_sum (ExprCallArg eca) { return TYPE4 (eca, get_sum); }
bool fct_max (ExprCallArg eca) { return TYPE4 (eca, get_max); }
bool fct_min (ExprCallArg eca) { return TYPE4 (eca, get_min); }

