/*
    structures.c
*/

#include <_stdio.h>
#include <component.h>
#include <structures.h>


/***********************************************************************************************************/

static Structure expression_structure;

Expression *expression_new (const Expression *newExpr)
{
    Expression *expression;
    STRUCT_NEW (Expression, expression, expression_structure, "Expression")
    if(newExpr!=NULL) memcpy (expression, newExpr, sizeof(Expression));
    else printf("In expression_new(): newExpr==NULL.\n");
    return expression;
}

static void expression_destroy (Expression *expression)
{
    if(expression==NULL) return;
    if(expression->deleted) return;
    value_free(expression->constant);
    lchar_free (expression->name);
    STRUCT_DEL (Expression, expression, expression_structure)
    expression->deleted = true;
}

void expression_remove (Expression *expression)
{
    Expression *expr, *expr_next;
    if(expression==NULL) return;
    if(expression->deleted) return;

    // recursively remove children
    for(expr = expression->headChild; expr != NULL; expr = expr_next)
    {
        expr_next = expr->nextChild;
        expression_remove(expr);
    }
    expression_destroy (expression);
}



static long expression_to_valueSt_2 (Expression *expr, Value* vst)
{
    long len;

    if(expr->headChild==NULL)
    {
        len = 1;
        setString(*vst, expr->name);
        expr->name = NULL; // or do strcpy:
        //lstr=NULL;
        //if(expr->name!=NULL)
        //    astrcpy33(&lstr, expr->name);
        //setString(vst, lstr);
    }
    else
    {   len = 1;
        for(expr = expr->headChild; expr != NULL; expr = expr->nextChild)
            len += expression_to_valueSt_2 (expr, vst+len);
        setSepto2(*vst, len);
    }
    return len;
}

Value* expression_to_valueSt (Expression *expression)
{
    if(expression==NULL) return NULL;
    //expression_tree_print(expression); // in expresion.h

    long len = expression_to_valueSt_2 (expression, (Value*)error_message);

    Value* vst = value_alloc (len);
    memcpy(vst, error_message, len*sizeof(Value));

    //printf("expression_to_valueSt = '%s'\n", vst_to_str(vst));
    return vst;
}


/***********************************************************************************************************/

static Structure depend_structure;

Depend *depend_new (Component *component)
{
    Depend *depend;
    STRUCT_NEW (Depend, depend, depend_structure, "Depend")
    depend->component = component;
    return depend;
}

void depend_destroy (Depend *depend)
{ STRUCT_DEL (Depend, depend, depend_structure) }

void depend_remove (Depend* depend_head)
{ struct_destroy_all (Depend, depend_head, depend_destroy) }

void depend_print (const Depend* depend)
{   int i=0;
    for( ; depend != NULL; depend = depend->next, i++)
        printf("(%p -> %p = '%s') ", depend, depend->component,
        (depend->component) ? CST13(GCN(depend->component)) : "NULL");
    printf("Total = %d.\n", i);
}

int  depend_count (const Depend* depend)
{ int i=0; for( ; depend!=NULL; depend = depend->next) i++; return i; }

static bool depend_check (const Depend* depend)
{ for( ; depend!=NULL; depend = depend->next) if(depend->component==NULL) return 0; return 1; }


/***********************************************************************************************************/

static Structure component_structure;

Component *component_new()
{
    Component *component;
    STRUCT_NEW (Component, component, component_structure, "Component")
    return component;
}

void component_destroy (Component *component)
{
    if(component==NULL) return;
    //if(component->dependOnMe != NULL)
    //{ component_print("Error in component_destroy(): dependOnMe != NULL", component);
        depend_remove (component->dependOnMe);
    lchar_free (component->name1);
    lchar_free (component->name2);
    lchar_free (component->strExpr1);
    lchar_free (component->strExpr2);
    value_free (component->cname2);
    value_free (component->parameter1);
    value_free (component->parameter2);
    depend_denotify (component->depend1, component);
    depend_remove (component->depend1);
    depend_remove (component->depend2);
    expression_remove (component->root1);
    expression_remove (component->root2);
    STRUCT_DEL (Component, component, component_structure)
}

void component_remove (Component* component_head)
{ struct_destroy_all (Component, component_head, component_destroy) }


/***********************************************************************************************************/

static Structure container_structure;

Container *container_new ()
{
    Container *container;
    STRUCT_NEW (Container, container, container_structure, "Container")
    return container;
}

void container_destroy (Container *container)
{
    if(container == NULL) return;
    component_remove (container->first);
    lchar_free (container->name);
    lchar_free (container->type);
    lchar_free (container->text);
    value_free (container->result);
    STRUCT_DEL (Container, container, container_structure)
}


/***********************************************************************************************************/


void expression_print (const char* text, const Expression *expr)
{
    if(!expr) return;
    printf("\n");
    printf("%s expression      = %p\n"   , text, expr);
    printf("%s exr->name       = '%s'\n" , text, CST13(expr->name));
    printf("%s exr->line:coln  = %d:%d\n", text, expr->name->line, expr->name->coln);
    printf("%s exr->ID         = %d\n"   , text, expr->ID);
    printf("%s exr->type       = 0x%x\n" , text, expr->type);
    printf("%s exr->previous   = 0x%x\n" , text, expr->previous);
    printf("%s exr->precedence = %d\n"   , text, expr->precedence);
    printf("%s exr->evaluate   = %p\n"   , text, expr->evaluate);
    printf("%s exr->constant   = '%s'\n" , text, vst_to_str(expr->constant));
    printf("\n\n");
}



void component_print (const char* text, const Component  *component)
{
    int line, coln;
    char str[1000];
    if(component==NULL) return;

    sprintf1 (str, "\n"); puts1(str);
    sprintf1 (str, "%s component              = %p\n", text, component); puts1(str);

    if(component->name1==NULL) line=coln=-99; else { line = component->name1->line; coln = component->name1->coln; }
    sprintf1 (str, "%s component->name1       = '%s' %d:%d\n" , text, CST13(component->name1), line, coln); puts1(str);

    if(component->name2==NULL) line=coln=-99; else { line = component->name2->line; coln = component->name2->coln; }
    sprintf1 (str, "%s component->name2       = '%s' %d:%d\n" , text, CST13(component->name2), line, coln); puts1(str);

    if(component->strExpr1==NULL) line=coln=-99; else { line = component->strExpr1->line; coln = component->strExpr1->coln; }
    sprintf1 (str, "%s component->expr1       = '%s' %d:%d\n" , text, CST13(component->strExpr1), line, coln); puts1(str);

    if(component->strExpr2==NULL) line=coln=-99; else { line = component->strExpr2->line; coln = component->strExpr2->coln; }
    sprintf1 (str, "%s component->expr2       = '%s' %d:%d\n" , text, CST13(component->strExpr2), line, coln); puts1(str);

    sprintf1 (str, "%s component->depend1     = %p  count=%d  check=%d\n"   , text, component->depend1, depend_count(component->depend1), depend_check(component->depend1)); puts1(str);
    sprintf1 (str, "%s component->depend2     = %p  count=%d  check=%d\n"   , text, component->depend2, depend_count(component->depend2), depend_check(component->depend2)); puts1(str);
    sprintf1 (str, "%s component->dependOnMe  = %p  count=%d  check=%d\n"   , text, component->dependOnMe, depend_count(component->dependOnMe), depend_check(component->dependOnMe)); puts1(str);

    sprintf1 (str, "%s component->root1       = %p\n", text, component->root1); puts1(str);
    sprintf1 (str, "%s component->root2       = %p\n", text, component->root2); puts1(str);

    sprintf1 (str, "%s component->parameters1 = %p\n", text, component->parameter1); puts1(str);
    sprintf1 (str, "%s component->parameters2 = %p\n", text, component->parameter2); puts1(str);

    sprintf1 (str, "%s component->state       = %d\n", text, component->state); puts1(str);

    sprintf1 (str, "------component_print is done------\n"); puts1(str);
}



void container_print  (const char* text, const Container  *container)
{
    int count;
    char str[1000];
    const Component *component;
    if(container==NULL) return;

    sprintf1(str, "---------------------------------------------CONTAINER PRINT START\n"); puts1(str);
    sprintf1(str, "\n"); puts1(str);

    sprintf1(str, "%s container             = %p\n"  , text, container); puts1(str);

    sprintf1(str, "%s container->name       = '%s'\n", text, CST13(container->name)); puts1(str);

    for(count=0, component = container->first; component != NULL; component = component->next, count++)
        component_print(text, component);

    sprintf1(str, "%s container components  = %d\n"  , text, count); puts1(str);

    sprintf1(str, "---------------------------------------------CONTAINER PRINT STOP\n\n"); puts1(str);
}


/***********************************************************************************************************/


mchar* dependency_get_traverse (const char* text,
                                const int spaces,
                                const Component *component,
                                mchar* output)
{
    int i;
    mchar* out;
    const Depend* depend;
    static mchar _output[1000];

    if(output==NULL) { output = _output; } out = output;
    if(component==NULL) { *out = 0; return output; }

        {
        strcpy21 (out, text); out += strlen2(out);
        for(i=0; i < spaces; i++) *out++ = ' ';

        //component = depend->component;
        sprintf2 (out, CST21("'%s'."), CST23(component->container->name)); out += strlen2(out);
        strcpy23 (out, ((component->name2==NULL) ? component->name1 : component->name2)); out += strlen2(out);

        *out++ = '\r';
        *out++ = '\n';
        }

    for(depend = component->dependOnMe; depend != NULL; depend = depend->next)
    {
        if(depend->component->container == component->container) continue;

        dependency_get_traverse (text, spaces+4, depend->component, out); out += strlen2(out);
    }
    *out = 0;
    return output;
}

