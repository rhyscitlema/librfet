/*
    expression.c
*/

#include <_stdio.h>
#include <_string.h>
#include <_texts.h>
#include <component.h>
#include <expression.h>
#include "operations.h"



static Expression *expression_new (const Expression *newExpr)
{
    Expression *expression = (Expression*)_malloc(sizeof(Expression)); memory_alloc("Expression");
    if(newExpr!=NULL) *expression = *newExpr;
    else printf("Software Error in expression_new(): newExpr==NULL.\n");
    return expression;
}

static const Expression* get_the_item (const wchar* strExpr)
{
    int j;
    for(j=0; character_type[j].ID; j++)
        if(0==strcmp22 (TWSF(character_type[j].ID), strExpr))
            return &character_type[j];
    strcpy21(errorMessage(), "Software Error: In get_the_item(): return==NULL.");
    return NULL;
}


static const Expression* precede_with_times (const lchar** strExpr, lchar** str)
{
    astrcpy32(str, TWSF(Times_1));
    lchar_copy(*str, *strExpr);
    return get_the_item (TWSF(Times_2));
}


static bool isCharacterType (wchar c)
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
    wchar c;
    int size=0;
    const lchar* start = *strExpr;
    while(1)
    {
        c = (*strExpr)->wchr;
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
                Component *component)
{
    int j, k, m, id, size;
    wchar c;
    lchar* str2 = NULL;
    const lchar* start;
    int currentItem_type;
    Expression *nextItem=NULL;
    const Expression *tempItem;

    if(currentItem==NULL) currentItem_type=0;
    else currentItem_type = currentItem->type;

    wchar* errormessage = errorMessage();


    // Task 1: skip the spaces
    while(1)
    {
        c = (*strExpr)->wchr;
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
    if(isDigit(c) || (c=='.' && isDigit((*strExpr)->next->wchr)))
    {
        if((currentItem_type & CLOSEBRACKET))
            GNI_RETURN(precede_with_times(strExpr,str))

        j=0;
        if(c=='0')
        {   c = (*strExpr)->next->wchr;
            if(c=='b' || c=='o' || c=='x') j=1;
        }
        while(1)
        {
            c = (*strExpr)->wchr;
            if(c!='.' && (j ? !isAlpNu(c) : !isDigit(c))) break;
            (*strExpr) = (*strExpr)->next; size++;
        }
        astrcpy33S (str, start, size);

        wchar* mstr = ErrStr0;
        strcpy23S (mstr, start, size);
        value t = StrToVal(mstr);

        if(getType(t))
        {   number_type.constant = t;
            GNI_RETURN(&number_type)
        }
        else
        {   set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\n\\5", *str, getNotval(t));
            GNI_RETURN(NULL)
        }
    }


    // Task 4: check if the next item is a character_type
    m=0;
    for(j=0; ; j++)
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

    // Task 5: check if next item is of the form .comp_name for the container.component call
    if(c=='.')
    {
        start = *strExpr;
        (*strExpr) = (*strExpr)->next;          // skip '.'
        size = extract_string(strExpr, str);    // get comp_name
        if(size==0)
        {   set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nExpected a name directly after '.'.", start);
            GNI_RETURN(NULL)
        }
        astrcpy33S(str, start, size+1); // get '.comp_name'
        nextItem = &contcall_type;

        // check if there is a '(' after component_name
        for(start = *strExpr; isSpace(start->wchr); start = lchar_next(start));
        if(start->wchr == '(')
             nextItem->type = AFUNCTION;
        else nextItem->type = AVARIABLE;
        GNI_RETURN(nextItem)
    }


    // Task 6: check if the next item is a string
    if(c=='"')
    {
        // if opening '"' has been found
        start = (*strExpr)->next;
        size = 0;
        bool escape=0;
        lchar* lstr = (lchar*)errormessage;
        while(true)
        {
            (*strExpr) = (*strExpr)->next;
            c = (*strExpr)->wchr;
            if(escape && c=='#')
            {   (*strExpr) = lchar_next(*strExpr);
                c = (*strExpr)->wchr;
            }
            if(!escape && c=='\\') { escape=1; continue; }

            if(!escape && c=='"')
            {
                // if closing '"' has been found
                (*strExpr) = (*strExpr)->next; // skip it
                astrcpy33S(str, lstr, size);
                GNI_RETURN(&string_type)
            }
            if(c==0) // if end of content
            {
                // end reached without finding a closing '"'
                astrcpy31 (str, "\"");
                lchar_copy(*str, *strExpr);
                set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nExpected a closing '\\4'.", *str);
                GNI_RETURN(NULL)
            }
            lstr[size++] = (**strExpr);
            lstr[size-1].next = &lstr[size]; // NOTE: no 'prev' is set as 'assumed' as not needed.
            escape = 0;
        }
    }

    // Task 7: collect the next item as a string and put inside 'str'
    size = extract_string (strExpr, str);

    // Task 8: check if the next item is an opr_word_type (a named operator)
    for(j=0; opr_word_type[j].ID != 0; j++)
        if(0==strcmp23 (TWSF(opr_word_type[j].ID), *str))
            GNI_RETURN(&opr_word_type[j])

    // Task 9: this portion is used only by component_extract() in component.c
    if(component==NULL) GNI_RETURN(&variable_type)

    // Task 10: check if '*' must be placed before the next item
    if(currentItem_type & ALEAVE)
    {
        *strExpr = lchar_goto (*strExpr, -size);
        GNI_RETURN(precede_with_times(strExpr,str))
    }

    // Task 11: check if next item is the current_type
    if(0==strcmp31(*str, "current"))
    {
        for(tempItem = currentItem; tempItem != NULL; tempItem = tempItem->parent)
            if(tempItem->ID == Replacement)
                GNI_RETURN(&current_type)
    }

    // Task 12: check if next item is a function parameter
    if(valueSt_get_position ((int*)parameter_type.param, c_para(component), *str))
        GNI_RETURN(&parameter_type)

    // Task 13: check if next item is a user-defined variable or function
    Component *tcomponent = component_find(c_container(component), *str, errormessage, 0);
    if(!tcomponent)
    {
        Component *globalcomp = component_find(container_find(0, CST31("|GLOBAL.MFET"), 0,0,0), *str, 0,0);
        if(globalcomp && c_access(globalcomp) > ACCESS_PRIVATE) tcomponent = globalcomp;
    }
    if(tcomponent)
    {
        tcomponent = component_parse(tcomponent);
        if(tcomponent)
        {
            if(c_para(tcomponent)==NULL)
                 nextItem = &variable_type;
            else nextItem = &function_type;
            nextItem->call_comp = tcomponent;
            depend_add (component, tcomponent);
            GNI_RETURN(nextItem)
        }
    }

    // Task 14: check if the next item is an outsider_type (like the time 't')
    j = outsider_getID(*str);
    if(j>0)
    {   SET_OUTSIDER_ID(&outsider_type, j);
        if(j & OUTSIDER_ISA_FUNCTION)
             outsider_type.type = SkipClimbUp | AFUNCTION;
        else outsider_type.type = SkipClimbUp | AVARIABLE;
        GNI_RETURN(&outsider_type)
    }

    // Task 15: check if the next item is a word_type (a named constant or function)
    for(j=0; word_type[j].ID != 0; j++)
        if(0==strcmp23 (TWSF(word_type[j].ID), *str))
            GNI_RETURN(&word_type[j])

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



Expression *parseExpression (const lchar* strExpr, Component *component)
{
    lchar* str=NULL;
    const wchar* mstr;
    wchar bracketOpened[100];
    int bracketOpenedCount = 0;
    const Expression *nextItem;
    const Expression *currentItem;
    Expression *root=NULL, *current;
    assert(strExpr!=NULL);

    nextItem = get_the_item (TWSF(Opened_Bracket_1));
    if(nextItem==NULL) return NULL;
    root = expression_new (nextItem);
    root->parent = NULL;
    root->headChild = NULL;
    current = root;
    currentItem = root;

    wchar* errormessage = errorMessage();
    while(1)
    {
        nextItem = get_next_item (&strExpr, &str, currentItem, component);
        if(nextItem==NULL)
        {
            if(strExpr->wchr==0 && str==NULL) break; // if end of expression
            current=NULL; break;  // else an error occured while getting next item
        }

        // This portion is used only by component_extract() in component.c
        if(component==NULL
        && !(nextItem->type & OPENBRACKET) && !(nextItem->type & CLOSEBRACKET)
        && !(nextItem->type & ACOMMA) && !(nextItem->type & AVARIABLE))
        { set_message(errormessage, TWST(IsNot_ValueStructure), str); current=NULL; break; }

        //if(0==strcmp(str, "|")) do something else;
        if(nextItem->type & OPENBRACKET) bracketOpened [bracketOpenedCount++] = str->wchr;
        if(nextItem->type & CLOSEBRACKET)
        {
            bracketOpened[99] = '\0';
            mstr = &bracketOpened[98];
            if(bracketOpenedCount > 0)
            {   bracketOpened[98] = OpenedToClosedBracket (bracketOpened [--bracketOpenedCount]);
                if(*mstr != str->wchr)
                { set_message(errormessage, TWST(Bracket_Match_Invalid), str, mstr); current=NULL; break; }
            }
            else{ bracketOpened[98] = ClosedToOpenedBracket (str->wchr);
                  set_message(errormessage, TWST(Bracket_Match_None), str, mstr); current=NULL; break; }
        }

        if(!(nextItem->previous & currentItem->type))   // if invalid syntax
        { set_message(errormessage, TWST(Invalid_Expression_Syntax), str); current=NULL; break; }

        current = insertNew (current, nextItem);
        if(!current) break;
        if(!current->component)
            current->component = component;

        if(nextItem->type & CLOSEBRACKET)
            currentItem = nextItem;
        else
        {   currentItem = current;
            if(current->name==NULL) { current->name = str; str=NULL; }
        }
    }

    if(!current);

    else if(bracketOpenedCount > 0) // if there is an opened bracket not yet closed
    {
        if(bracketOpenedCount==1)
             mstr = TWST(Lacking_Bracket);
        else mstr = TWST(Lacking_Brackets);
        set_message(errormessage, mstr, strExpr, TIS2(0,bracketOpenedCount));
        current=NULL;
    }
    else if(!(currentItem->type & ALEAVE)) // if invalid end of expression
    {
        set_message(errormessage, TWST(Invalid_Expression_End), strExpr);
        current=NULL;
    }
    else
    {   current = root->headChild;
        root->headChild = NULL;
        current->parent = NULL;
    }
    expression_remove(root);
    lchar_free(str);
    return current;
}



// Perform tree traversal given the starting node
static void traverse_recursively (const Expression *expression, int spaces)
{
    int i;
    wchar mstr[20];
    const Expression *expr;
    if(expression==NULL) return;

    // print the headChild first
    expr = expression->headChild;
    traverse_recursively (expr, spaces+10);

    for(i=0; i < spaces; i++) putc2(' ');

    if(expr)
    {
        expr = expression;
        printf("p=%d ", expr->precedence);
        if(expr->name) printf("s='%s' %d:%d", CST13(expr->name), expr->name->line, expr->name->coln);
        printf("\n");

        expr = expression->headChild;
        for(expr = expr->nextSibling; expr != NULL; expr = expr->nextSibling)
            traverse_recursively (expr, spaces+10);
    }
    else
    {
        expr = expression;
        VstToStr ((value*)expr->param, mstr, 0,-1,-1,-1);
        printf("v='%s' s='%s'", CST12(mstr), CST13(expr->name));
        if(expr->name==NULL) printf(" ?:?");
        else printf(" %d:%d", expr->name->line, expr->name->coln);
        printf("\n");
    }
}

// Perform tree traversal given the starting node
void expression_tree_print (const Expression *expression)
{
    puts1("------------------------------ expression_tree_print start\n");
    traverse_recursively (expression, 0);
    puts1("______________________________ expression_tree_print stop\n");
}

