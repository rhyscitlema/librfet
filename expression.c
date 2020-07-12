/*
    expression.c
*/

#include <stdio.h> // for printf() only
#include <_stdio.h>
#include "outsider.h"
#include "component.h"
#include "expression.h"
#include "operations.h"


static Expression *expression_new (const Expression *newExpr)
{
    Expression *expression = (Expression*)_malloc(sizeof(Expression), "Expression");
    assert(newExpr!=NULL); if(newExpr!=NULL) *expression = *newExpr;
    return expression;
}

static void expression_destroy (Expression *expression)
{
    assert(expression!=NULL);
    assert(expression->deleted==false);
    if(expression->deleted) return;
    str3_free(expression->name);
    memset(expression, 0, sizeof(Expression));
    expression->deleted = true;
    _free(expression, "Expression");
}

static void expression_remove (Expression *expression)
{
    Expression *expr, *next;
    if(expression==NULL) return;
    assert(expression->deleted==false);

    for(expr = expression->headChild; expr != NULL; expr = next)
    {
        next = expr->nextSibling;
        expression_remove(expr);
    }
    expression_destroy(expression);
}


static const Expression* get_the_item (enum ID_TWSF ID)
{
    int j;
    for(j=0; char_type[j].ID; j++)
        if(char_type[j].ID == ID)
            return &char_type[j];
    assert(false && "get_the_item() failed");
    return NULL;
}

static const Expression* precede_with_times (const_Str3* strExpr, const_Str3* str)
{
    str->ptr = strExpr->ptr;
    str->end = strExpr->ptr->next;
    return get_the_item(Oper_mul0);
}

static bool isCharacterType (wchar c)
{
    switch(c)
    {
    case '#': case '`': case '~': case '!': case '@':
    case '$': case '%': case '^': case '&': case '*':
    case '(': case ')': case '-': case '+': case '=':
    case '|': case '<': case '>': case ',': case '.':
    case '/': case '?': case ';': case ':': case '"':
    case '[': case ']': case '{': case '}':
    case '\'': case L'•': return true;
    default: return false;
    }
    // Note: '_' and '\\' will return false
}

/* size=0 if a space_type or char_type is found */
static void extract_string (const_Str3* strExpr, const_Str3* str)
{
    str->ptr = strExpr->ptr;
    while(!strEnd3(*strExpr))
    {
        wchar c = sChar(*strExpr);
        if(isSpace(c)) break;
        if(isCharacterType(c)) break;
        (*strExpr) = sNext(*strExpr);
    }
    str->end = strExpr->ptr;
}

static bool get_parameter_position (int *position, const_value v, const_Str3 param)
{
    if(!v || *v<2) return false;
    v++; // skip v[0], it is = 1 + number of paras
    const_value end = vNext(v);
    int p=0;
    while(v<end)
    {
        int a = *v >> 28;
        assert(a==VAL_VECTOR || isStr2(v));
        if(a==VAL_VECTOR) v+=2; // skip vector header
        else if(0==strcmp32(param, getStr2(v))) { *position = p; break; }
        else { v = vNext(v); p++; }
    }
    return (v<end);
}



const Expression* get_next_item (
                value stack,
                const_Str3* strExpr,
                const_Str3* str,
                const Expression *currentItem,
                Component *const component)
{
    int j, m;
    long size;
    wchar c;
    Str3 lstr;
    int currentItem_type;
    Expression *nextItem=NULL;
    const Expression *tempItem;
    const_Str2 argv[2];
    assert(stack && strExpr && str);
    if(!(stack && strExpr && str)) return NULL;

    // Task 1: prepare to start looking for the next item
    if(currentItem==NULL) currentItem_type=0;
    else currentItem_type = currentItem->type;
    nextItem = NULL;

    // Task 2: skip the spaces
    while(true)
    {
        if(strEnd3(*strExpr)) // if end of expression
        { *str=C37(NULL); return NULL; }
        c = sChar(*strExpr);
        if(!isSpace(c) && c!='#') break;
        (*strExpr) = strNext3(*strExpr);
    }
    *str = *strExpr;


    // Task 3: check if the next item is a number
    if(isDigit(c) || (c=='.' && isDigit(sChar(sNext(*strExpr)))))
    {
        if(currentItem_type & CLOSEBRACKET)
            return precede_with_times(strExpr,str);

        j=false;
        if(c=='0')
        {   c = sChar(sNext(*strExpr));
            if(c=='b' || c=='o' || c=='x') j=true;
        }
        while(true)
        {
            c = sChar(*strExpr);
            if(c!='.' && c!='_' &&
               (j ? !isAlpNu(c) : !isDigit(c))) break;
            (*strExpr) = sNext(*strExpr);
        }
        str->end = strExpr->ptr;
        return &constant_type;
    }


    // Task 4: check if the next item is a char_type
    m=0;
    for(j=0; ; j++)
    {
        enum ID_TWSF id = char_type[j].ID;
        if(id==0) break;

        int i, k = strlen2(TWSF(id));
        if(!k || k<m) continue; // not k<=m due to precedence order

        lstr = *strExpr;
        for(i=0; i<k && !strEnd3(lstr); i++) lstr = sNext(lstr);
        str->ptr = strExpr->ptr;
        str->end = lstr.ptr;

        if(0==strcmp32(*str, TWSF(id)))
        {
            if((char_type[j].type & OPENBRACKET)
            && (currentItem_type & ALEAVE))
            {
                if(id==OpenedBracket3)
                     return get_the_item(Oper_indexing);
                else return precede_with_times(strExpr,str);    // else do a multiplication if something like a(1) => a*(1)
            }

            if(char_type[j].previous & currentItem_type         // check if valid syntax
            || nextItem==NULL                                   // or if nothing previously found
            || !(nextItem->previous & currentItem_type))        // or if what was previously found was invalid
            {
                m = k;
                nextItem = &char_type[j];
            }
        }
    }
    if(nextItem!=NULL)
    {
        str->ptr = strExpr->ptr;
        for(j=0; j<m; j++) *strExpr = sNext(*strExpr);
        str->end = strExpr->ptr;
        return nextItem;
    }

    // Task 5: check if next item is of the form .comp_name for the 'container.component' call
    if(c=='.')
    {
        lstr = *strExpr;
        (*strExpr) = sNext(*strExpr);   // skip '.'

        extract_string(strExpr, str);   // get comp_name
        if(strEnd3(*str))
        {
            argv[0] = L"Error at (%2,%3) in %4:\r\nExpected a name directly after '.'.";
            setMessageE(stack, 0, 1, argv, lstr);
            return NULL;
        }
        str->ptr = lstr.ptr;

        // check if there is a '(' after component_name
        for(lstr = *strExpr; !strEnd3(lstr); lstr = strNext3(lstr))
            if(!isSpace(sChar(lstr))) break;
        if(!strEnd3(lstr) && sChar(lstr) == '(')
             dot_call_type.type = AFUNCTION;
        else dot_call_type.type = AVARIABLE;
        return &dot_call_type;
    }


    // Task 6: check if the next item is a character or string
    if(c=='"' || c=='\'')
    {
        wchar chr = c;
        bool escape = false;
        while(true)
        {
            (*strExpr) = sNext(*strExpr);
            if(strEnd3(*strExpr))
            {
                // end reached without finding a closing " or '
                argv[1] = (chr=='"' ? L"\"" : L"'");
                argv[0] = L"Error at (%2,%3) in %4:\r\nExpected a closing '%s'.";
                setMessageE(stack, 0, 2, argv, *str);
                return NULL;
            }
            c = sChar(*strExpr);
            if(!escape) {
                if(c==chr) break;
                if(c=='\\') escape = true;
            } else escape = false;
        }
        (*strExpr) = sNext(*strExpr); // skip the ' or " found
        str->end = strExpr->ptr;
        return &constant_type;
    }


    // Task 7: collect the next item as a string and put inside 'str'
    extract_string(strExpr, str);
    size = strlen3(*str);

    if(size > MAX_NAME_LEN)
    {
        argv[0] = L"Error at (%2,%3) in %4:\r\nWord of length %5 is too long.";
        argv[1] = TIS2(0,size);
        setMessageE(stack, 0, 2, argv, *str);
        return NULL;
    }
    else if(size==0)
    {
        str->end = sNext(*strExpr).ptr;
        argv[0] = L"Error on '%s' at (%s,%s) in %s:\r\nExpected a valid word instead.";
        setMessageE(stack, 0, 1, argv, *str);
        return NULL;
    }

    // Task 8: check if the next item is an oper_type (a named operator)
    for(j=0; oper_type[j].ID != 0; j++)
        if(0==strcmp32(*str, TWSF(oper_type[j].ID)))
            return &oper_type[j];

    // Task 9: this portion is used only by component_extract() in component.c
    if(component==NULL) return &paramter_type;

    // Task 10: check if '*' must be placed before the next item
    if(currentItem_type & ALEAVE)
    {
        strExpr->ptr = str->ptr; // go back as thought no loading took place
        return precede_with_times(strExpr,str);
    }

    // Task 11: check if next item is the repl_lhs_type
    if(0==strcmp31(*str, "LHS"))
    {
        for(tempItem = currentItem; tempItem != NULL; tempItem = tempItem->parent)
            if(tempItem->ID == Replacement)
                return &repl_lhs_type;
    }

    // Task 12: check if next item is a function parameter
    if(get_parameter_position (&paramter_type.param, c_para(component), *str))
        return &paramter_type;

    // Task 13: check if next item is a user-defined variable or function
    Component *compcall = component_find(stack, c_container(component), *str, 0);
    if(!compcall)
    {
        value v = vnext(stack); assert(VERROR(v));
        Component *globalcomp =
            component_find(v,
                container_find(v, 0, C31("|GLOBAL.RFET"), 0)
                , *str, 0);
        if(globalcomp && c_access(globalcomp) > ACCESS_PRIVATE)
            compcall = globalcomp;
    }
    if(compcall)
    {
        if(!component_parse(stack, compcall))
            return NULL; // if error

        depend_add (component, compcall);

        if(*c_para(compcall)) // if component is a function
        {
            const_Str3 s = *strExpr;
            while(true)
            {
                c = strEnd3(s) ? 0 : sChar(s);
                if(isSpace(c)) { s = sNext(s); continue; }
                if(!(c==0 || c==',' || c==';' || c==')' || c=='}')) break;
                compname_type.call_comp = compcall;
                return &compname_type;
            }
            var_func_type.type = SkipClimbUp | AFUNCTION;
        }
        else var_func_type.type = SkipClimbUp | AVARIABLE;
        var_func_type.call_comp = compcall;
        return &var_func_type;
    }

    // Task 14: check if the next item is an outsider_type (like 'time')
    j = outsider_getID(*str);
    if(j)
    {   outsider_type.outsider = j;
        if(j & OUTSIDER_ISA_FUNCTION)
             outsider_type.type = SkipClimbUp | AFUNCTION;
        else outsider_type.type = SkipClimbUp | AVARIABLE;
        return &outsider_type;
    }

    // Task 15: check if the next item is a word_type (a named constant or function)
    for(j=0; word_type[j].ID != 0; j++) // TODO: TODO: move this to component find
        if(0==strcmp32(*str, TWSF(word_type[j].ID)))
            return &word_type[j];

    // Task 16: check if "this"
    if(0==strcmp32(*str, TWSF(Constant_this)))
    {   compname_type.call_comp = c_container(component);
        return &compname_type;
    }

    return NULL;
}



// Add the given new item to the expression tree
static Expression *insertNew (Expression *current, const Expression *newItem)
{
    Expression *node;

    if(!(newItem->type & SkipClimbUp))
    {
        /* step 4: climb up */
        if(!(newItem->type & RightAssoct))
        {
            /* for left-associative */
            while(current->precedence >= newItem->precedence)
                current = current->parent;
        }
        else
        {
            /* for right-associative */
            while(current->precedence > newItem->precedence)
                current = current->parent;
        }
    }

    if(newItem->type & CLOSEBRACKET)
    {
        /* step 5: delete the '(' node */
        if(current->prevSibling != NULL)
        {   current->prevSibling->nextSibling = current->headChild;
            current->headChild->prevSibling = current->prevSibling;
        }
        else current->parent->headChild = current->headChild;
        current->headChild->parent = current->parent;
        current->parent->lastChild = current->headChild;

        node = current->parent;
        current->headChild = NULL;          // this line is to make sure that
        current->prevSibling = NULL;
        expression_remove(current);        // only this 'current' node is removed

        /* step 6: Set the 'current node' to be the parent node */
        current = node;
    }

    else if(!(newItem->type & ACOMMA && current->type & ACOMMA))
    {
        /* step 5: add the new node */
        node = expression_new (newItem);
        node->parent = current;

        if(current->lastChild == NULL)
            current->headChild = node;
        else
        {
            if(newItem->type & SkipClimbUp)
            {
                current->lastChild->nextSibling = node;
                node->prevSibling = current->lastChild;
            }
            else
            {
                node->prevSibling = current->lastChild->prevSibling;

                if(node->prevSibling != NULL)               // if current node has more than one child node.
                    node->prevSibling->nextSibling = node;  // Notice the replacement done on this line.
                else current->headChild = node;

                current->lastChild->parent = node;
                node->headChild = current->lastChild;
                node->lastChild = current->lastChild;
                node->headChild->prevSibling = NULL;
            }
        }
        current->lastChild = node;

        /* step 6: Set the 'current node' to be the new node */
        current = node;
    }
    return current;
}

static value expression_to_operation (const Expression *expression, value stack);



value parseExpression (value stack, const_Str3 strExpr, Component *component)
{
    const_value start = stack;
    const_Str3 str = C37(NULL);
    const_Str2 argv[3];
    wchar bracketOpened[100];
    int bracketOpenedCount = 0;
    const Expression *nextItem;
    const Expression *currentItem;
    Expression *root=NULL, *current;
    assert(strExpr.ptr && stack);
    if(!(strExpr.ptr && stack)) return stack;

    nextItem = get_the_item(OpenedBracket1);
    if(nextItem==NULL) return NULL;
    root = expression_new(nextItem);
    root->name.ptr = strExpr.ptr;
    root->name.end = strExpr.ptr;
    root->parent = NULL;
    root->headChild = NULL;
    current = root;
    currentItem = root;

    while(true)
    {
        nextItem = get_next_item (stack, &strExpr, &str, currentItem, component);
        if(nextItem==NULL)
        {
            if(strEnd3(str) && strEnd3(strExpr)) break; // if end of expression
            stack = vnext(stack);   // go to after VAL_MESSAGE to prepare for return
            break;                  // on an error occured while getting next item
        }
        assert(nextItem->ID != EndOfStatement);

        // This portion is used only by component_extract() in component.c
        if(component==NULL
        && !(nextItem->type & OPENBRACKET)
        && !(nextItem->type & CLOSEBRACKET)
        && !(nextItem->type & ACOMMA)
        && !(nextItem->type & APARAMTER))
        {
            argv[0] = TWST(IsNot_ValueStructure);
            stack = setMessageE(stack, 0, 1, argv, str);
            break;
        }

        //if(0==strcmp(str, "|")) do something else;
        if(nextItem->type & OPENBRACKET) bracketOpened[bracketOpenedCount++] = sChar(str);
        if(nextItem->type & CLOSEBRACKET)
        {
            bracketOpened[99] = '\0';
            const wchar* wstr = &bracketOpened[98];
            if(bracketOpenedCount > 0)
            {
                bracketOpened[98] = OpenedToClosedBracket (bracketOpened [--bracketOpenedCount]);
                if(*wstr != sChar(str))
                {
                    argv[0] = TWST(Bracket_Match_Invalid);
                    argv[1] = wstr;
                    stack = setMessageE(stack, 0, 2, argv, str);
                    break;
                }
            }
            else
            {   bracketOpened[98] = ClosedToOpenedBracket(sChar(str));
                argv[0] = TWST(Bracket_Match_None);
                argv[1] = wstr;
                stack = setMessageE(stack, 0, 2, argv, str);
                break;
            }
        }

        if(!(nextItem->previous & currentItem->type))   // if invalid syntax
        {
            argv[0] = TWST(Invalid_Expression_Syntax);
            stack = setMessageE(stack, 0, 1, argv, str);
            break;
        }

        current = insertNew (current, nextItem);
        if(!current) break; // if error
        if(!current->component)
            current->component = component;

        if(nextItem->type & CLOSEBRACKET)
            currentItem = nextItem;
        else
        {   currentItem = current;
            if(strEnd3(current->name)) current->name = str;
        }

        if(nextItem->ID == Replacement){
            assert(current->parent->type & OPENBRACKET);
            current->name.end = current->name.ptr;
            current->name.ptr = current->parent->name.end;
        }
    }

    if(start != stack); // if error

    else if(bracketOpenedCount > 0) // if there is an opened bracket not yet closed
    {
        argv[2] = (bracketOpenedCount==1) ? L"" : L"s";
        argv[1] = TIS2(0,bracketOpenedCount);
        argv[0] = TWST(Lacking_Bracket);
        stack = setMessageE(stack, 0, 3, argv, strExpr);
    }
    else if(!(currentItem->type & ALEAVE)) // if invalid end of expression
    {
        argv[0] = TWST(Invalid_Expression_End);
        stack = setMessageE(stack, 0, 1, argv, strExpr);
    }
    else stack = expression_to_operation(root->headChild, stack);

    expression_remove(root); root=NULL;
    assert(stack == vnext(vPrev(stack)));
    return stack;
}



// Perform tree traversal given the starting node
static void expression_tree_print (const Expression *expression, int spaces)
{
    if(expression==NULL) return;
    if(spaces==0) printf("------------------------------ expression_tree_print start\n");

    // print the headChild first
    const Expression *expr = expression->headChild;
    expression_tree_print(expr, spaces+10);

    int i; for(i=0; i < spaces; i++) putc2(' ');

    if(strEnd3(expression->name))
        printf("?:?");
    else printf("s='%s' %d:%d",
            C13(expression->name),
            sLine(expression->name),
            sColn(expression->name));
    if(expr) printf(" p=%d ", expression->precedence);
    printf("\n");

    if(expr)
        for(expr = expr->nextSibling; expr != NULL; expr = expr->nextSibling)
            expression_tree_print(expr, spaces+10);

    if(spaces==0) printf("______________________________ expression_tree_print stop\n");
}


static value set_error (value v, const_Str3 name)
{
    const_Str2 argv[2];
    argv[0] = L"Error on %s at (%s,%s) in %s:\r\n%s";
    argv[1] = getMessage(vPrev(v));
    return setMessageE(v, 0, 2, argv, name);
}

static inline value setOpers (value v, int oper, int size)
{
    assert(!(oper & ~0x0FFF) && !(size & ~0xFFFF));
    *v = (VAL_OPERAT<<28) | (oper<<16) | size;
    return v+1+size;
}


#define EXPR_TO_OPER(expr) v = expression_to_operation(expr, v); if(!*v) { ok=false; break; }

static value expression_to_operation (const Expression *expression, value stack)
{
    if(!stack) return NULL;
    value w, v = stack;
    const Expression *expr;
    enum ID_TWSF ID = expression->ID;
    bool ok = true;

    bool root = !expression->parent || !expression->parent->parent;
    if(root) {
        expression_tree_print(NULL/*expression*/, 0);
        v += 2; // reserve space for vector header
    }
    if(ID==SET_PARAMTER && expression->component==NULL) ID = SET_CONSTANT;
    switch(ID)
    {
    case SET_CONSTANT:
        w = StrToVal(v+1, C23(expression->name));
        if(!VERROR(w)) v = setOpers(v, ID, w-1-v-1);
        else { v = set_error(w, expression->name); ok=false; }
        break;

    case SET_COMPNAME:
        container_path_name(v+1, expression->call_comp);
        v = setOpers(v, SET_CONSTANT, vNEXT(v+1)-(v+1));
        break;

    case SET_PARAMTER:
        setOpers(v, ID, expression->param & 0xFFFF);
        v+=1; break;

    case SET_REPL_LHS:
        for(expr = expression; expr; expr = expr->parent)
            if(expr->ID == Replacement) break;
        assert(expr->ptr_to_lhs!=NULL);
        v[1] = v - expr->ptr_to_lhs;
        v = setOpers(v, ID, 1);
        break;

    case Replacement:
        v = setOpers(v, OperJustJump, 0);
        ((Expression*)(size_t)expression)->ptr_to_lhs = v;
        w = v;
        EXPR_TO_OPER(expression->headChild)
        assert(w+1000>v); while(w < v+1) w += 1000;
        v = setOpers(v, OperJustJump, w-v-1);

        v = w+1+4+2+2+1+1;
        EXPR_TO_OPER(expression->lastChild)
        v[1] = v-w;
        v = setOpers(v, ReplaceRecord, 1);

        *(const_Str3*)(w+1) = expression->name;
        *(Component**)(w+1+4) = expression->component;
        *(long*)(w+1+4+2) = 0; // for evaluation_instance
        w[1+4+2+2  ] = w - expression->ptr_to_lhs; // offset to LHS code
        w[1+4+2+2+1] = v - w; // offset to after the RHS code
        setOpers(w, ID, 4+2+2+1+1);
        break;

    case ConditionAsk:
        EXPR_TO_OPER(expression->headChild)
        w = v;
        v += 1+4+1; // reserve spaces for ConditionAsk
        expr = expression->lastChild;
        if(expr->ID != ConditionChoose)
        {
            v = setErrorE(v, TWST(Expect_Choice_After), expression->name);
            ok=false; break;
        }
        EXPR_TO_OPER(expr->headChild)

        *(const_Str3*)(w+1) = expression->name;
        *(w+1+4) = v+1 - w; // set amount by which to skip
        setOpers(w, ConditionAsk, 5);

        w = v;
        v += 1; // reserve a space for ConditionChoose|Jump
        EXPR_TO_OPER(expr->lastChild)
        setOpers(w, OperJustJump, v-w-1);
        break;

    case ConditionChoose:
        v = setErrorE(v, TWST(Expect_Question_Before), expression->name);
        ok=false; break;

    case Function_try:
        *(const_Str3*)(v+1) = expression->name;
        v = setOpers(v, ID, 4);
        expr = expression->headChild;
        if(expr->ID==CommaSeparator)
            expr = expr->headChild;
        while(true)
        {   EXPR_TO_OPER(expr)
            expr = expr->nextSibling;
            if(expr) v = setOpers(v, Function_try_that , 0);
            else   { v = setOpers(v, Function_try_catch, 0); break; }
        }
        break;

    case CommaSeparator:
        w = 0;
        for(expr = expression->headChild; expr; expr = expr->nextSibling)
        {   EXPR_TO_OPER(expr) w++;
        } if(!ok) break;
        *(v+1) = w-(value)0; // it just works!
        v = setOpers(v, ID, 1);
        break;

    case SET_DOT_CALL: // first know what this operator does
        EXPR_TO_OPER(expression->lastChild)
        w = (expression->headChild != expression->lastChild)?v:0;
        if(w){ EXPR_TO_OPER(expression->headChild) }
        *(const_Str3*)(v+1) = expression->name;
        *(Component**)(v+1+4) = expression->component;
        if(expression->type & AFUNCTION) ID = SET_DOT_FUNC;
        v = setOpers(v, ID, 4+2);
        break;

    case Constant_true:
    case Constant_false:
    case Constant_e_2_718_:
    case Constant_pi_3_141_:
    case SQRT_of_Neg_One:
        v = setOpers(v, ID, 0); break;

    default:
        for(expr = expression->headChild; expr; expr = expr->nextSibling)
        {   EXPR_TO_OPER(expr)
        } if(!ok) break;

        *(const_Str3*)(v+1) = expression->name;
        w = v+1+4;

        switch(ID)
        {
        case SET_OUTSIDER: *w = expression->outsider; w+=1; break;
        case SET_VAR_FUNC: *(Component**)w = expression->call_comp; w+=2; break;
        default: break;
        }
        v = setOpers(v, ID, w-v-1);
        break;
    }

    if(!root) *v = ok;
    else // on error, copy error message
        if(!ok) v = vpcopy(stack, v);
    else
    {   w = v+1;
        Component* c = expression->component;
        if(c) { *(Component**)w = c; w+=2; }

        v = setOpers(v, 0, w-v-1); // ID=0 means END
        setVector(stack, 1, v-stack-2);
        v = setOffset(v, v-stack);
    }
    return v;
}

