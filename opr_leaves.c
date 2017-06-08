/*
    opr_leaves.c

    These are the main leave nodes:
     - a number
     - a usertext
     - a parameter
     - a function
     - a variable
     - a contcall
*/

bool set_number (ExprCallArg eca)
{
    value y={0};
    if(eca.expression->ID == SET_NUMBER)           { y = eca.expression->constant; }
    if(eca.expression->ID == Constant_E_2_718_)    { y = setSmaInt(+1); y = _exp_e(y); }
    if(eca.expression->ID == Constant_PI_3_141_)   { y = setSmaInt(-1); y = _acos(y);  }
    if(eca.expression->ID == SQRT_of_Negative_One) { y = setSmaCo2(0,1); }
    *eca.stack = y;
    INDEPENDENT(eca.expression, eca.stack, 1);
    return 1;
}

bool set_string (ExprCallArg eca)
{
    lchar* lstr=NULL;
    astrcpy33(&lstr, eca.expression->name);
    *eca.stack = setString(lstr);
    INDEPENDENT(eca.expression, eca.stack, 1);
    return 1;
}



bool set_parameter (ExprCallArg eca)
{
    const value* arg = eca.garg->argument;
    if(!arg) { *eca.stack = setSmaInt(0); return 1; }

    const int* p = eca.expression->param;
    int i, j;
    for(i=1; i<=p[0]; i++)
    {
        arg += 1;
        for(j=0; j<p[i]; j++)
            arg += VST_LEN(arg);
    }
    eca.expression->independent = 2;
    vst_shift(eca.stack, arg);
    return 1;
}



bool set_function (ExprCallArg eca)
{
    Expression* expression = eca.expression;
    Component* comp = expression->call_comp;
    if(comp->replace>0)
    {   Component* c = component_find(eca.garg->caller, expression->name, eca.garg->message, 0); assert(c!=NULL);
        if(c && c_access(c)==ACCESS_REPLACE) comp = c;
    }
    GeneralArg garg;
    Expression *expr;
    const value* para;
    bool success = false;
    while(true) // not a loop
    {
        // get the argument to function call
        eca.expression = expression->lastChild;
        if(!expression_evaluate(eca)) break;
        value* arg = eca.stack;

        para = c_para(comp);
        expr = c_root(comp);
        assert(expr!=NULL);

        if(valueSt_compare(para, arg)>3) // if matching is not valid
        {
            wchar* errormessage = eca.garg->message;
            VstToStr(arg , ErrStr0, 1,-1,-1,-1);
            VstToStr(para, ErrStr1, 1,-1,-1,-1);
            set_message(errormessage, TWST(Argument_vs_Parameter), expression->name, ErrStr0, ErrStr1);
            break;
        }

        garg = *eca.garg;
        if(!c_isaf(comp) || expression->ID == SET_CONTCALL)
            garg.caller = c_container(comp);

        garg.argument = arg;
        eca.garg = &garg;
        eca.expression = expr;
        eca.stack += VST_LEN(arg);

        if(!expression_evaluate(eca)) break;
        vst_shift(arg, eca.stack);

        success = true;
        break;
    }
    return success;
}



bool set_variable (ExprCallArg eca)
{
    Expression* expression = eca.expression;
    Component* comp = expression->call_comp;
    if(comp->replace>0)
    {   Component* c = component_find(eca.garg->caller, expression->name, eca.garg->message, 0); assert(c!=NULL);
        if(c && c_access(c)==ACCESS_REPLACE) comp = c;
    }
    GeneralArg garg;
    Expression *expr;
    bool success = false;
    while(true) // not a loop
    {
        expr = c_root(comp);
        assert(expr!=NULL);

        if(!c_isaf(comp) || expression->ID == SET_CONTCALL)
        {   garg = *eca.garg;
            garg.caller = c_container(comp);
            eca.garg = &garg;
        }
        eca.expression = expr;
        if(!expression_evaluate(eca)) break;

        // before VERY careful when implementing the part below
        if(0/*first time*/)
            INDEPENDENT(expression, eca.stack, expr->independent);

        success = true;
        break;
    }
    return success;
}



bool set_contcall (ExprCallArg eca)
{
    Expression* expression = eca.expression;
    const lchar* name = expression->name;
    Expression *head = expression->headChild;

    wchar* errormessage = eca.garg->message;
    lchar* lstr = NULL;
    bool success = false;
    while(true) // not a loop
    {
        eca.expression = head;
        if(!expression_evaluate(eca)) break;

        if(!isString(*eca.stack))
        { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\nLeft to operation must evaluate to a string.", name); break; }

        lstr = getString(*eca.stack);
        const lchar* path = lstr;
        Container* current = c_container(expression->component);

        if(path && path->wchr=='|') // if path = a file name
        {
            Container* container = container_find(current, path, errormessage, 0,0);
            if(container==NULL)
            {   lchar* name2=NULL; astrcpy33(&name2, path);
                container = container_parse(NULL, name2, NULL); name2=NULL;
                if(container && !dependence_parse()) container=NULL;
                dependence_finalise(container!=NULL);
                if(container==NULL) break;
            }
        }

        // construct pathname
        int len = strlen3(path);
        lchar pathname[MAX_PATH_SIZE];
        set_lchar_array(pathname, len + strlen3(name) + 1);
        strcpy33(pathname, path);
        strcpy33(pathname+len, name);   // name has '.' at its beginning
        pathname[len].wchr = '|';       // change from '.' to '|'

        Component* component = container_find(current, pathname, errormessage, 0,0);
        if(component==NULL) break;
        expression->call_comp = component;
        eca.expression = expression;

        bool afunction = expression->lastChild != head;
        if(c_para(component)==NULL)
        {
            if(afunction) { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4': expected a variable not a function.", name); break; }
            success = set_variable(eca);

            //depend_add(expression->component, component); // TODO: inside set_contcall: allow this line only if this happens only once
            //expression->evaluate = set_variable;
        }
        else
        {   if(!afunction) { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4': expected a function not a variable.", name); break; }
            success = set_function(eca);
        }
        break;
    }
    //lchar_free(lstr); // TODO: inside set_contcall(): is from getString(v)
    return success;
}

