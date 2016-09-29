/*
    opr_leaves.c

    These are the main leave nodes:
     - a number
     - a constant
     - a userstring
     - a parameter
     - a function
     - a variable
*/

Value* set_number (Expression* expression, const Value* argument)
{
    Value* out = value_alloc(1);
    if(expression->ID == 0)                    { *out = *(Value*)expression->param; }
    if(expression->ID == Constant_E_2_718_)    { setSmaInt(*out,1);   *out = _exp_e(*out); }
    if(expression->ID == Constant_PI_3_141_)   { setSmaInt(*out,-1);  *out = _acos(*out);  }
    if(expression->ID == SQRT_of_Negative_One) { *out = setSmaCo2(0,1); }
    expression->independent = 1;
    INDEPENDENT(expression)
    return out;
}

Value* set_userstring (Expression *expression, const Value* argument)
{
    Value* out = value_alloc(1);
    lchar* lstr=NULL;
    astrcpy33(&lstr, expression->name);
    setString(*out, lstr);
    expression->independent = 1;
    INDEPENDENT(expression)
    return out;
}




Value* set_parameter (Expression *expression, const Value* arg)
{
    if(!arg)
    {   Value* out = value_alloc(1);
        setSmaFlt(*out, 0);
        return out;
    }

    const int* p = expression->param;
    int i, j;
    for(i=1; i<=p[0]; i++)
    {
        arg += 1;
        for(j=0; j<p[i]; j++)
            arg += VST_LEN(arg);
    }
    expression->independent = 2;
    //return (Value*)arg;
    return value_copy(arg);
}



Value* set_function (Expression* expression, const Value* argument)
{
    const Value* parameter;
    Expression *expr;
    Value* out=NULL;

    // get the argument to function call
    expr = expression->lastChild;
    Value* arg = expression_evaluate(expr, argument);
    if(arg==NULL) return NULL;

    if(expression->component->root2 == NULL)
    {
        parameter = expression->component->parameter1;
        expr = expression->component->root1;
        if(expr==NULL) set_error_message (CST21("Software Error in set_function():\r\nIn \\1 at \\2:\\3 on '\\4':\r\nexpression->component->roo1->roo2=NULL."), expression->name);
    }
    else
    {   parameter = expression->component->parameter2;
        expr = expression->component->root2;
    }

    if(expr)
    {
        // check matching between parameter and argument value structures
        if(valueSt_compare(parameter, arg)&1) // if matching is not one-to-
        {
            VstToStr(arg      , error_message+1000, 1,-1,-1,-1);
            VstToStr(parameter, error_message+2000, 1,-1,-1,-1);
            set_error_message (TWST(Argument_vs_Parameter), expression->name,
                               error_message+1000, error_message+2000);
        }
        else out = expression_evaluate(expr, arg);
    }
    value_free(arg);
    return out;
}



Value* set_variable (Expression* expression, const Value* argument)
{
    Expression *expr = expression->component->root2;
    if(expr==NULL) expr = expression->component->root1;
    if(expr==NULL) { set_error_message(CST21("Software Error in set_variable():\r\nIn \\1 at \\2:\\3 on '\\4':\r\nexpression->component->roo1->roo2=NULL."), expression->name); return NULL; }
    Value* out = expression_evaluate(expr, argument);

    // before VERY careful when implementing the part below
    if(0/*first time*/ && out)
    {   expression->independent = expr->independent;
        INDEPENDENT(expression) // first make a copy of out
    }
    return out;
}



Value* set_contcall (Expression* expression, const Value* argument)
{
    const lchar *lstr, *name = expression->name;
    Expression *head = expression->headChild;
    Value* in = expression_evaluate(head, argument);
    if(in==NULL) return NULL;
    Value* out=NULL;

    while(1) // not a loop
    {
        if(!isString(*in))
        { set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nLeft to operation must evaluate to a string."), name); break; }
        lstr = getString(*in);

        Container* tcontainer = NULL;
        if(!container_add (&tcontainer, lstr, NULL)) break;

        // parse all affected components
        if(!dependence_parse()) break;

        Component* tcomponent = component_find (tcontainer, name->next); // skip '.' so to get comp_name
        if(tcomponent==NULL)
        {
            strcpy22 (error_message+1000, error_message);
            set_error_message (CST21("Error in \\1 at \\2:\\3 on \"\\5\"\\4:\r\n\\6"), name, CST23(lstr), error_message+1000);
            break;
        }

        if( tcontainer != expression->component->container &&
            ((tcomponent->name2==NULL) ? tcomponent->isprivate1 : tcomponent->isprivate2))
        { set_error_message (CST21("Error in \\1 at \\2:\\3: \"\\5\"\\4 has private access."), name, CST23(lstr)); break; }

        tcomponent = component_parse (tcomponent, tcontainer);
        if(tcomponent==NULL)
        {
            strcpy22 (error_message+1000, error_message);
            set_error_message (CST21("Error in \\1 at \\2:\\3 on \"\\5\"\\4:\r\n\\6"), name, CST23(lstr), error_message+1000);
            break;
        }

        bool afunction = expression->lastChild != head;
        if(tcomponent->parameter1==NULL && tcomponent->parameter2==NULL)
        {
            if(afunction) { set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4': expected a variable not a function."), name); break; }
            expression->evaluate = set_variable;
        }
        else
        {   if(!afunction) { set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4': expected a function not a variable."), name); break; }
            expression->evaluate = set_function;
        }
        depend_add (expression->component, tcomponent);
        expression->component = tcomponent;

        out = expression_evaluate(expression, argument);
        break;
    }
    dependence_finalise(out!=NULL);
    value_free(in);
    return out;
}

