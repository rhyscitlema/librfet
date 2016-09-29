/*
    expression.c

    Let '>' to mean 'overwrites', then:
    function parameter > user-defined variable/function > software-defined variable/function
*/

#include <_stdio.h>
#include <_string.h>
#include <_texts.h>
#include <expression.h>
#include <component.h>



static const Expression* get_the_item (const mchar* strExpr)
{
    int j;
    for(j=0; character_type[j].ID; j++)
        if(0==strcmp22 (TWSF(character_type[j].ID), strExpr))
            return &character_type[j];
    strcpy21(error_message, "Software Error: In get_the_item(): return==NULL.");
    return NULL;
}


static const Expression* precede_with_times (const lchar** strExpr, lchar** str)
{
    astrcpy32(str, TWSF(Times_1));
    (*str)->file = (*strExpr)->file;
    (*str)->line = (*strExpr)->line;
    (*str)->coln = (*strExpr)->coln;
    return get_the_item (TWSF(Times_2));
}


static bool isCharacterType (mchar c)
{
    int j;
    if(c=='_') return false;
    for(j=0; character_type[j].ID != 0; j++)
        if(TWSF(character_type[j].ID)[0] == c)
            return true;
    return false;
}


/* size=0 if a space_type or character_type is found */
static int extract_string (const lchar** strExpr, lchar** str)
{
    mchar c;
    int size=0;
    const lchar* start = *strExpr;
    while(1)
    {
        c = (*strExpr)->mchr;
        if(c==0 || isSpace(c)) break;
        if(isCharacterType(c)) break;
        if(c=='#') break; // if start of comment
        (*strExpr) = (*strExpr)->next;
        size++;
    }
    if(str) astrcpy33S (str, start, size);
    return size;
}



#define GNI_RETURN(ret) \
{   lchar_free (str2); \
    return ret; \
}

const Expression* get_next_item (
                const lchar** strExpr,
                lchar** str,
                const Expression *currentItem,
                const Value* parameter,
                Component *component,
                Container *container)
{
    int j, k, m, id, size;
    mchar c;
    mchar* str1;
    lchar* str2 = NULL;
    const lchar* start;
    const mchar *dPt;
    int currentItem_type;
    Component *tcomponent;
    Expression *nextItem=NULL;
    const Expression *tempItem;

    if(currentItem==NULL) currentItem_type=0;
    else currentItem_type = currentItem->type;


    // Task 1: skip the spaces
    while(1)
    {
        c = (*strExpr)->mchr;
        if(c==0) // if end of expression
        { lchar_free(*str); *str=NULL; return NULL; }
        if(!isSpace(c) && c!='#') break;
        (*strExpr) = lchar_next(*strExpr);
    }

    // Task 2: prepare to start looking for the next item
    nextItem = NULL;
    start = *strExpr;
    size = 0;


    // Task 3: check if the next item is a number
    dPt = TWSF(DecimalPoint);
    if(c==*dPt && isDigit((*strExpr)->next->mchr))
    { set_error_message (CST21("Error in \\1 at \\2:\\3:\r\nNumber must not begin with a decimal point."), *strExpr); GNI_RETURN(NULL) }

    if(isDigit(c))
    {
        if((currentItem_type & CLOSEBRACKET))
            GNI_RETURN(precede_with_times(strExpr,str))

        j=0; if(c=='0') { c = (*strExpr)->next->mchr; if(c=='b' || c=='o' || c=='x') j=1; }
        while(1)
        {
            c = (*strExpr)->mchr;
            if(c!=*dPt && (j ? !isAlpNu(c) : !isDigit(c))) break;
            (*strExpr) = (*strExpr)->next; size++;
        }
        str1 = error_message + 1000;
        strcpy23S (str1, start, size);
        astrcpy33S (str, start, size);

        Value t = StrToVal(str1);
        if(t.type)
        {   *(Value*)number_type.param = t;
            GNI_RETURN(&number_type)
        }
        else
        {   strcpy22(error_message+1000, error_message);
            set_error_message (CST21("Error in \\1 at \\2:\\3:\r\n\\5"), *str, error_message+1000);
            GNI_RETURN(NULL)
        }
    }


    // Task 4: check if the next item is a character_type
    m = 0;
    for(j = 0; ; j++)
    {
        id = character_type[j].ID;
        if(id==0) break;
        k = strlen2(TWSF(id));
        if(0==strcmp23S (TWSF(id), *strExpr,  k)
        &&  k >= m )
        {
            if((character_type[j].type & OPENBRACKET)
            && (currentItem_type & ALEAVE))
            {
                if(id==Opened_Bracket_3)
                {
                    astrcpy33S (str, *strExpr, k);
                    GNI_RETURN(&indexing_type)
                }
                else GNI_RETURN(precede_with_times(strExpr,str))    // else do a multiplication if something like a(1) => a*(1)
            }

            if(character_type[j].previous & currentItem_type    // check if valid syntax
            || nextItem==NULL                                   // or if nothing previously found
            || !(nextItem->previous & currentItem_type))        // or if what was previously found was invalid
            {
                m = k;
                nextItem = &character_type[j];
            }
        }
    }
    if(nextItem!=NULL)
    {
        astrcpy33S (str, *strExpr, m);
        (*strExpr) = lchar_goto (*strExpr, m);
        GNI_RETURN(nextItem)
    }

    if(c==*TWSF(DecimalPoint))
    {
        start = *strExpr;
        (*strExpr) = (*strExpr)->next;          // skip '.'
        size = extract_string(strExpr, str);    // get component name
        if(size==0)
        {   set_error_message (CST21("Error in \\1 at \\2:\\3:\r\nExpected a name directly after '.'."), start);
            GNI_RETURN(NULL)
        }
        astrcpy33S(str, start, size+1); // get '.comp_name'
        nextItem = &contcall_type;
        nextItem->component = component;

        for(start = *strExpr; isSpace(start->mchr); start = lchar_next(start));
        if(start->mchr == *TWSF(Opened_Bracket_1)) // if next will be '('
             nextItem->type = AFUNCTION;
        else nextItem->type = AVARIABLE;
        GNI_RETURN(nextItem)
    }


    // Task 4: check if the next item is a string
    if(c==*TWSF(DoubleQuote))
    {
        // if opening DoubleQuote '"' has been found
        start = (*strExpr)->next;
        size = 0;
        bool escape=0;
        while(true)
        {
            (*strExpr) = (*strExpr)->next;
            c = (*strExpr)->mchr;
            if(!escape && c=='#')
            {   (*strExpr) = lchar_next(*strExpr);
                c = (*strExpr)->mchr;
            }
            if(!escape && c=='\\') { escape=1; continue; }

            if(!escape && c==*TWSF(DoubleQuote))
            {
                // if closing DoubleQuote '"' has been found
                (*strExpr) = (*strExpr)->next; // skip it
                astrcpy32S (str, error_message, size);
                (*str)->file = start->file;
                (*str)->line = start->line;
                (*str)->coln = start->coln;
                GNI_RETURN(&userstring_type)
            }
            if(c==0) // if end of content
            {
                // end reached without finding a DoubleQuote
                astrcpy32 (str, TWSF(DoubleQuote));
                (*str)->file = (*strExpr)->file;
                (*str)->line = (*strExpr)->line;
                (*str)->coln = (*strExpr)->coln;
                set_error_message (CST21("Error in \\1 at \\2:\\3:\r\nExpected a closing '\\4'."), *str);
                GNI_RETURN(NULL)
            }
            error_message[size++] = c;
            escape = 0;
        }
    }

    // Task 5: collect the next item as a string and put inside 'str'
    size = extract_string (strExpr, str);

    // Task 7: check if the next item is an opr_str_type (a named operator)
    for(j=0; opr_str_type[j].ID != 0; j++)
        if(0==strcmp23 (TWSF(opr_str_type[j].ID), *str))
            GNI_RETURN(&opr_str_type[j])

    // Task 9: this portion is used only by component_extract() in component.c
    if(parameter==NULL && component==NULL && container==NULL) GNI_RETURN(&variable_type)

    // Task 8: check if '*' must be placed before the next item
    if(currentItem_type & ALEAVE)
    {
        *strExpr = lchar_goto (*strExpr, -size);
        GNI_RETURN(precede_with_times(strExpr,str))
    }

    // Task 9: check if next item is the current_type
    if(0==strcmp31(*str, "current"))
    {
        for(tempItem = currentItem; tempItem != NULL; tempItem = tempItem->parent)
            if(tempItem->evaluate == opr_replace)
                GNI_RETURN(&current_type)
    }

    // Task 10: check if next item is a function parameter
    if(valueSt_get_position ((int*)parameter_type.param, parameter, *str))
        GNI_RETURN(&parameter_type)

    // Task 12: check if next item is a user-defined variable or function
    tcomponent = component_find (container, *str);
    if(tcomponent == NULL)
    {
        strcpy22 (error_message+500, error_message);
        set_error_message (CST21("Error in \\1 at \\2:\\3:\r\n\\5"), *str, error_message+500);
    }
    else
    {
        tcomponent = component_parse (tcomponent, container);
        if(tcomponent)
        {
            if(tcomponent->parameter1==NULL && tcomponent->parameter2==NULL)
                 nextItem = &variable_type;
            else nextItem = &function_type;
            nextItem->component = tcomponent;
            depend_add (component, tcomponent);
            GNI_RETURN(nextItem)
        }
    }

    // Task 8: check if the next item is an outsider_type (like the time 't')
    j = outsider_getID (*str);
    if(j>0)
    {   outsider_type.ID = j;
        GNI_RETURN(&outsider_type)
    }

    // Task 7: check if the next item is a string_type (a named constant or function)
    for(j=0; string_type[j].ID != 0; j++)
        if(0==strcmp23 (TWSF(string_type[j].ID), *str))
            GNI_RETURN(&string_type[j])

    GNI_RETURN(NULL)
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
        if(current->prevChild != NULL)
        {   current->prevChild->nextChild = current->headChild;
            current->headChild->prevChild = current->prevChild; // may not be necessary
        }
        else current->parent->headChild = current->headChild;
        current->headChild->parent = current->parent;
        current->parent->lastChild = current->headChild;

        node = current->parent;
        current->headChild = NULL;          // this line is to make sure that
        current->prevChild = NULL;
        expression_remove (current);        // only this 'current' node is removed

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
                current->lastChild->nextChild = node;
                node->prevChild = current->lastChild;
            }
            else
            {
                node->prevChild = current->lastChild->prevChild;

                if(node->prevChild != NULL)             // if current node has more than one child node.
                    node->prevChild->nextChild = node;  // Notice the replacement done on this line.
                else current->headChild = node;

                current->lastChild->parent = node;
                node->headChild = current->lastChild;
                node->lastChild = current->lastChild;
                node->headChild->prevChild = NULL;
            }
        }
        current->lastChild = node;

        /* step 6: Set the 'current node' to be the new node */
        current = node;
    }
    return current;
}



#define PE_RETURN(ret) { expression_remove(root); lchar_free(str); return ret; }

Expression *parseExpression (const lchar* strExpr,
                             const Value* parameter,
                             Component *component,
                             Container *container)
{
    lchar* str=NULL;
    const mchar* mstr;
    mchar bracketOpened[100];
    int bracketOpenedCount = 0;
    const Expression *nextItem;
    const Expression *currentItem;
    Expression *root=NULL, *current;

    if(strExpr==NULL) return NULL;
    strcpy21(error_message, "parseExpression() is started.\n");

    while(isSpace(strExpr->mchr)) strExpr = lchar_next(strExpr);
    if(strExpr->mchr==0) // if strExpr has no non-space character
    { set_error_message (TWST(Empty_Expression), strExpr); PE_RETURN(NULL) }

    nextItem = get_the_item (TWSF(Opened_Bracket_1));
    if(nextItem==NULL) return NULL;
    root = expression_new (nextItem);
    root->parent = NULL;
    root->headChild = NULL;
    current = root;
    currentItem = root;

    while(1)
    {
        nextItem = get_next_item (&strExpr, &str, currentItem, parameter, component, container);
        if(nextItem==NULL)
        {
            if(strExpr->mchr==0 && str==NULL) break; // if end of expression
            PE_RETURN(NULL)  // else an error occured while getting next item
        }

        // This portion is used only by component_extract() in component.c
        if((parameter==NULL && component==NULL && container==NULL)
        && !(nextItem->type & OPENBRACKET) && !(nextItem->type & CLOSEBRACKET)
        && !(nextItem->type & ACOMMA) && !(nextItem->type & AVARIABLE))
        { set_error_message (TWST(IsNot_ValueStructure), str); PE_RETURN(NULL) }

        //if(0==strcmp(str, "|")) do something else;
        if(nextItem->type & OPENBRACKET) bracketOpened [bracketOpenedCount++] = str->mchr;
        if(nextItem->type & CLOSEBRACKET)
        {
            bracketOpened[99] = '\0';
            mstr = &bracketOpened[98];
            if(bracketOpenedCount > 0)
            {   bracketOpened[98] = OpenedToClosedBracket (bracketOpened [--bracketOpenedCount]);
                if(*mstr != str->mchr)
                { set_error_message (TWST(Bracket_Match_Invalid), str, mstr); PE_RETURN(NULL) }
            }
            else{ bracketOpened[98] = ClosedToOpenedBracket (str->mchr);
                  set_error_message (TWST(Bracket_Match_None), str, mstr); PE_RETURN(NULL) }
        }

        if(!(nextItem->previous & currentItem->type))   // if invalid syntax
        { set_error_message (TWST(Invalid_Expression_Syntax), str); PE_RETURN(NULL) }

        current = insertNew (current, nextItem);
        if(current == NULL) PE_RETURN(NULL)
        if(current->evaluate == opr_replace) current->component = component;

        if(nextItem->type & CLOSEBRACKET) currentItem = nextItem;
        else
        {   currentItem = current;
            if(current->name==NULL) { current->name = str; str=NULL; }
        }
    }

    if(bracketOpenedCount > 0) // if there is an opened bracket not yet closed
    {
        if(bracketOpenedCount==1)
             mstr = TWST(Lacking_Bracket);
        else mstr = TWST(Lacking_Brackets);
        set_error_message (mstr, strExpr, TIS2(0,bracketOpenedCount));
        PE_RETURN(NULL)
    }

    if(!(currentItem->type & ALEAVE)) // if invalid end of expression
    { set_error_message (TWST(Invalid_Expression_End), strExpr); PE_RETURN(NULL) }

    current = root->headChild;
    root->headChild = NULL;
    current->parent = NULL;

    //expression_tree_print (current);
    strcpy21(error_message, "parseExpression() is done.\n");

    PE_RETURN(current)
}



// Perform tree traversal given the starting node
static void traverse_recursively (const Expression *expression, int spaces)
{
    int i;
    mchar mstr[20];
    const Expression *expr;
    if(expression==NULL) return;

    expr = expression->headChild;
    traverse_recursively (expr, spaces+10);

    expr = expression;
    for(i=0; i < spaces; i++) putc2(' ');

    if(expr->headChild != NULL)
    {
        printf("p=%d s='%s'", expr->precedence, CST13(expr->name));
        if(expr->name==NULL) printf(" ?:?");
        else printf(" %d:%d", expr->name->line, expr->name->coln);
        printf("\n");
    }
    else
    {
        VstToStr ((Value*)expr->param, mstr, 0,-1,-1,-1);
        printf("v='%s' s='%s'", CST12(mstr), CST13(expr->name));
        if(expr->name==NULL) printf(" ?:?");
        else printf(" %d:%d", expr->name->line, expr->name->coln);
        printf("\n");
    }
    expr = expression->headChild;
    if(expr==NULL) return;
    for(expr = expr->nextChild; expr != NULL; expr = expr->nextChild)
        traverse_recursively (expr, spaces+10);
}

// Perform tree traversal given the starting node
void expression_tree_print (const Expression *expression)
{
    puts1("------------------------------ expression_tree_print start\n");
    traverse_recursively (expression, 0);
    puts1("______________________________ expression_tree_print stop\n");
}

