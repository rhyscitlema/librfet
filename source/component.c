/*
    component.c
*/

#include <_stdio.h>
#include <_texts.h>
#include <expression.h>
#include <component.h>


static Container *head_container = NULL;

enum CompState {
    ISPARSE, // I have been parsed
    DOPARSE, // I have changed, parse me
    NOPARSE, // I have not changed, do not parse me
    DELETED, // I have not been found in my container
    CREATED  // I have just been created
};


/**************************************************************************************************************/


Depend* depend_find (Depend* depend, const Component *component)
{
    for( ; depend != NULL; depend = depend->next)
        if(depend->component == component)
            return depend;
    return NULL;
}

/* read: I 'component', am depending on 'depending' */
void depend_add (Component* component, Component *depending)
{
    Depend *depend;
    if(component==NULL) return;
    if(depend_find (component->depend2, depending)) return;
    depend = depend_new (depending);
    DOUBLE_LINKED_LIST_NODE_ADD (component->depend2, depend);
}



/* read: I 'depend', am depended upon by 'component' */
static void depend_notify (Depend* depend, Component *component)
{
    Depend* tdepend;
    for( ; depend != NULL; depend = depend->next)
    {
        if(depend_find (depend->component->dependOnMe, component)) continue;
        tdepend = depend_new (component);
        DOUBLE_LINKED_LIST_NODE_ADD (depend->component->dependOnMe, tdepend);
    }
}

/* read: I 'depend', am not depended upon by 'component' */
void depend_denotify (Depend* depend, const Component *component)
{
    Depend* tdepend;
    for( ; depend != NULL; depend = depend->next)
    {
        tdepend = depend_find (depend->component->dependOnMe, component);
        if(tdepend==NULL) continue;
        DOUBLE_LINKED_LIST_NODE_REMOVE (depend->component->dependOnMe, tdepend);
        depend_destroy(tdepend);
    }
}



static void component_prepare (Component *component)
{
    if(component==NULL) return;

    if(component->state == NOPARSE)
    { component->state = DOPARSE; return; }

    if(component->state != ISPARSE) return;
    component->state = DOPARSE;

    astrcpy33 (&component->name2, component->name1);
    astrcpy33 (&component->strExpr2, component->strExpr1);
    avaluecpy (&component->parameter2, component->parameter1);
}



static void component_success (Component *component)
{
    if(component==NULL) return;
    if(component->state==DELETED)
    {
        DOUBLE_LINKED_LIST_NODE_REMOVE (component->container->first, component);
        component_destroy(component);
        return;
    }
    depend_denotify (component->depend1, component);
    depend_notify (component->depend2, component);
    lchar_free (component->name1); component->name1 = component->name2; component->name2 = NULL;
    lchar_free (component->strExpr1); component->strExpr1 = component->strExpr2; component->strExpr2 = NULL;
    value_free (component->parameter1); component->parameter1 = component->parameter2; component->parameter2 = NULL;
    depend_remove (component->depend1); component->depend1 = component->depend2; component->depend2 = NULL;
    if(component->root2 != NULL) { expression_remove(component->root1); component->root1 = component->root2; component->root2 = NULL; }
    component->isprivate1 = component->isprivate2; component->isprivate2 = 0;
    component->state = ISPARSE;
}



static void component_failure (Component *component)
{
    if(component==NULL) return;
    if(component->state==CREATED)
    {
        DOUBLE_LINKED_LIST_NODE_REMOVE (component->container->first, component);
        component_destroy(component);
        return;
    }
    lchar_free (component->name2); component->name2 = NULL;
    lchar_free (component->strExpr2); component->strExpr2 = NULL;
    value_free (component->parameter2); component->parameter2 = NULL;
    depend_remove (component->depend2); component->depend2 = NULL;
    expression_remove (component->root2); component->root2 = NULL;
    component->isprivate2 = 0;
    component->state = ISPARSE;
}


/**************************************************************************************************************/

static Component *dependenceHead = NULL;

static void dependence_add (Component* component)
{
    static Component *dependenceLast = NULL;
    Component *comp;
    for(comp = dependenceHead; comp != NULL; comp = comp->nextToParse)
        if(comp == component) return; // has already been added
    if(dependenceLast==NULL || dependenceHead==NULL) dependenceHead = component;
    else dependenceLast->nextToParse = component;
    dependenceLast = component;
    dependenceLast->nextToParse = NULL;
}

static void dependence_notify (Component *component)
{
    Depend *depend;
    if(component==NULL) return;
    dependence_add(component);
    component_prepare(component);
    for(depend = component->dependOnMe; depend != NULL; depend = depend->next)
        dependence_notify (depend->component);
}

bool dependence_parse ()
{
    Component *component;
    for(component = dependenceHead; component != NULL; component = component->nextToParse)
        if(component->state != DELETED)
            if(component_parse (component, component->container) == NULL)
                return false;
    return true;
}

bool dependence_evaluate ()
{
    Value* vst;
    Component *component;
    for(component = dependenceHead; component != NULL; component = component->nextToParse)
        if(component->state != DELETED)
        {   vst = component_evaluate(component,0,0);
            if(!vst) return false; else value_free(vst); }
    return true;
}

void dependence_finalise (bool success)
{
    Component *component, *component_next;
    for(component = dependenceHead; component != NULL; component = component_next)
    {
        component_next = component->nextToParse;
        (success) ? component_success(component) : component_failure(component);
    }
    dependenceHead = NULL;
    //structures_print_count();
}


/**************************************************************************************************************/
#define CNtoSTR(string, component) sprintf1(string, "Component_%p", component)


/* output: container->first = head of linked-list of components */
static bool component_extract (Container* container, const lchar* input)
{
    /*
    Here we separate components.
    A valid component is of the form:
        component_name = component_expression ;

    Components are statements, and so are separated by
    the End_of_Statement character ';'.
    If component_name is invalid then the whole statement is ignored.

    If preceded by 'private' then it is only
    accessible by members of its container.

    Below is the component extraction procedure:

    1st pass, set all component->state to:
        = ISPARSE

    2nd pass, set all component->state to:
        = DOPARSE if component has changed
        = NOPARSE if component has not changed
        = CREATED if component is new

    3rd pass, set all component->state to:
        = DELETED if component->state = ISPARSE

    4th pass, set all component->state to:
        = DOPARSE if component->state = NOPARSE
        'and' there was either a CREATED before.

    5th pass, notify for dependencies
    */
    int size, brackets;
    const Expression *nextItem;
    Expression* expression;
    Component *tcomponent;
    Component *component;
    const lchar* start;
    lchar* str = NULL;
    lchar* name = NULL;
    Value* cname = NULL;
    Value* parameter = NULL;
    const Value* end[20];
    int i, j, index[20];
    char temp[50]={0}, *tmp;
    bool notmainc=false, empty;
    bool created=false, ignore;
    bool error=false, isprivate;

    if(container==NULL || input==NULL) return false;

    if(isEmpty3(input,0)) { strcpy22 (error_message, TWST(Welcome_Message)); return false; }

    //1st pass, set all component->state to:
    //    = ISPARSE
    for(component = container->first; component != NULL; component = component->next)
        component->state = ISPARSE;


    while(input->mchr!=0) // while getting components
    {
        /* Step0: initialise main variables */
        ignore = true;
        isprivate = false;
        lchar_free(name); name = NULL;
        value_free(cname); cname = NULL;
        value_free(parameter); parameter=NULL;
        if(!notmainc) { astrcpy31(&name, "main"); ignore=false; } else{

        nextItem = get_next_item (&input, &str, 0,0,0,0);
        if(nextItem==NULL || input->mchr==0) break;


        /* Step1: Obtain component access control */
        if(0==strcmp31(str,"\\private"))
        {
            isprivate = true;
            nextItem = get_next_item (&input, &str, 0,0,0,0);
            if(nextItem==NULL || input->mchr==0) break;
        }


        /* Step2: Obtain component name */
        if(nextItem->type & AVARIABLE || 0==strcmp23 (TWSF(Opened_Bracket_2), str))
        {
            if(!(nextItem->type & AVARIABLE))
            {
                // if '{' then obtain name structure
                start=input->prev; brackets=1;
                while(input->mchr!=0)
                {
                    if(input->mchr == *TWSF(Opened_Bracket_2)) brackets++;
                    if(input->mchr == *TWSF(Closed_Bracket_2)) brackets--;
                    input = lchar_next(input);
                    if(brackets==0) break;
                }
                size = strlen3S(start, input);
                astrcpy33S (&str, start, size);
            }
            //printf("component_extract(): component name = '%s'\n", CST13(str));

            expression = parseExpression (str,0,0,0);
            if(expression==NULL) { error=true; break; }
            cname = expression_to_valueSt (expression);
            expression_remove(expression); expression=NULL;

            if(VST_LEN(cname) > 1) { name=str; str=NULL; } // if more than one component
            else { name = getString(*cname); getString(*cname)=NULL; value_free(cname); cname=NULL; }

            nextItem = get_next_item (&input, &str, 0,0,0,0);
            if(nextItem==NULL || input->mchr==0) break;


            /* Step3: Obtain component parameter */
            if(0==strcmp23 (TWSF(Opened_Bracket_1), str))
            {
                // if '(' then obtain parameter structure
                start=input->prev; brackets=1; empty=true;
                while(input->mchr!=0)
                {
                    if(input->mchr == *TWSF(Opened_Bracket_1)) brackets++;
                    if(input->mchr == *TWSF(Closed_Bracket_1)) brackets--;
                    input = lchar_next(input);
                    if(brackets==0) break;
                    if(!isSpace(input->mchr)) empty=false;
                }
                size = strlen3S(start, input);
                astrcpy33S (&str, start, size);

                if(!empty)
                {
                    expression = parseExpression (str,0,0,0);
                    if(expression==NULL) { error=true; break; }
                    parameter = expression_to_valueSt (expression);
                    expression_remove(expression); expression=NULL;
                }
                nextItem = get_next_item (&input, &str, 0,0,0,0);
                if(nextItem==NULL || input->mchr==0) break;
            }


            /* Step 4: final check for validity of entire component statement */
            if(0==strcmp23 (TWSF(Assignment), str)) ignore = false;
        }}

        start=input; size=0;
        if(!str || str->mchr != *TWSF(EndOfStatement))
        {
            while(input->mchr != 0 && input->mchr != *TWSF(EndOfStatement))
                input = lchar_next(input);
            size = strlen3S(start, input);
            if(input->mchr != 0) input = lchar_next(input); // skip EndOfStatement
        }
        if(ignore) continue; // if the component statement is not valid
        astrcpy33S (&str, start, size);


        /* 2nd pass, set all component->state to:
            = DOPARSE if component has changed
            = NOPARSE if component has not changed
            = CREATED if component is new
        */
        //printf("name = '%s'   ", CST13(name)); printf("para = '%s'   ", value_print(parameter)); printf("strE = '%s'\n", CST13(str));

        /* Step 6: record the extracted component */
        component = component_find (container, name);
        if(component==NULL)
        {
            component = component_new();
            DOUBLE_LINKED_LIST_NODE_ADD (container->first, component);
            component->container = container;
            component->state = CREATED;
            created = true;
        }
        else if(component->name2 != NULL)
        { set_error_message (TWST(Component_Already_Defined), name); error=true; break; }

        else if(0!=strcmp33 (component->strExpr1, str)
             || 0!=value_compare (component->parameter1, parameter))
             component->state = DOPARSE;
        else component->state = NOPARSE;

        component->name2 = name; name=NULL;
        component->strExpr2 = str; str=NULL;
        component->parameter2 = parameter; parameter=NULL;
        component->isprivate2 = isprivate;
        if(!notmainc) { notmainc=1; container->mainc = component; }


        /* Step 7: iterative tree-traversal to record sub-components of a component's result structure */
        if(VST_LEN(cname) > 1)
        {
            component->cname2 = cname;
            tcomponent = component;
            j = 0;
            while(1)
            {
                if(isString(*cname)) // if leave node is reached
                {
                    tmp = temp; tmp += CNtoSTR(tmp, tcomponent);
                    for(i=1; i<=j; i++) tmp += sprintf1(tmp, "[%d]", index[i]);
                    astrcpy31 (&str, temp);
                        {
                          name=str;
                          while(1)
                          { name->line = getString(*cname)->line;
                            name->coln = getString(*cname)->coln;
                            name->file = getString(*cname)->file;
                            if(name->mchr==0) break;
                            name = name->next;
                          } name=NULL;
                        }
                    astrcpy33 (&name, getString(*cname));
                    avaluecpy (&parameter, tcomponent->parameter2);

        /* Step 8: record the extracted sub-component */
        component = component_find (container, name);
        if(component==NULL)
        {
            component = component_new();
            DOUBLE_LINKED_LIST_NODE_ADD (container->first, component);
            component->container = container;
            component->state = CREATED;
            created = true;
        }
        else if(component->name2 != NULL)
        { set_error_message (TWST(Component_Already_Defined), name); error=true; break; }

        else if(0!=strcmp33 (component->strExpr1, str)
             || 0!=value_compare (component->parameter1, parameter))
             component->state = DOPARSE;
        else component->state = NOPARSE;

        component->name2 = name; name=NULL;
        component->strExpr2 = str; str=NULL;
        component->parameter2 = parameter; parameter=NULL;
        component->isprivate2 = isprivate;

                    cname++;
                    while(1)
                    {
                        if(j==0) break;
                        if(cname==end[j]) j--;
                        else { index[j]++; break; }
                    }
                    if(j==0) break;
                }
                else // if not leave node
                {
                    ++j;  index[j] = 0;
                    end[j] = cname + VST_LEN(cname);
                    cname++;
                }
            }
            cname=NULL; // note: do not free
            if(error) break;
        }
    }


    for(component = container->first; component != NULL; component = component->next)
    {
        //3rd pass, set all component->state to:
        //    = DELETED if component->state = ISPARSE
        if(component->state == ISPARSE)
           component->state =  DELETED;
    }
    for(component = container->first; component != NULL; component = component->next)
    {
        //4th pass, do not set component->state to:
        //    = DOPARSE if component->state = NOPARSE
        //    'and' there was no CREATED before.
        if(container->quickedit && !created && component->state == NOPARSE)
            dependence_add(component);

        //5th pass, notify for dependencies
        else dependence_notify(component);
    }

    lchar_free(str);
    lchar_free(name);
    value_free(cname);
    value_free(parameter);
    if(error) printf("component_extract():\n%s\n", CST12(error_message));
    return !error;
}



/* Create a new container and add to the linked-list of containers. */
/* Parse the container content into a linked-list of components. */
bool container_add (Container** container_ptr, const lchar* name, const lchar* content)
{
    bool success;
    Container *container;
    Component *component;
    Expression *expression;
    lchar* lcontent = NULL;
    mfet_init();

    if(container_ptr==NULL)
    { strcpy21(error_message, "Software Error in container_add: container_ptr==NULL."); return false; }

    if((name==NULL || name->mchr==0) && content==NULL)
    { strcpy21(error_message, "Empty input."); return false; }

    if(*container_ptr==NULL) *container_ptr = container_find(name);
    container = *container_ptr;
    bool isnew = container==NULL;

    if(content==NULL) // then name = "file name"
    {
        if(!isnew) return true;
        //if(!file_is_modified(name)) return true;

        mchar* mcontent=NULL;
        if(!Openfile (CST23(name), &mcontent, 0)) return false;
        // do not set error message, use that from container_find()

        astrcpy32(&lcontent, mcontent);
        mchar_free(mcontent);

        set_line_coln (lcontent, 1, 1);
        set_lchar_file (lcontent, submitSourceName(CST23(name)));
        content = lcontent;
    }

    if(isnew)
    {
        container = container_new();
        if(name) astrcpy33 (&container->name, name);
        if(lcontent!=NULL)
        {   astrcpy31 (&container->type, "File");
            lchar_free (container->text);
            container->text = lcontent; lcontent=NULL;
        }
    }

    success = component_extract (container, content);
    //container_print ("container_add", container);

    while(success) // not a loop
    {
        success = false;

        component = component_find (container, CST31("name"));
        if(component)
        {
            if(!component_parse(component, container)) break;

            expression = (component->root2==NULL) ? component->root1 : component->root2;
            if(expression==NULL || expression->evaluate != set_userstring)
            {
                name = (component->strExpr2==NULL) ? component->strExpr1 : component->strExpr2;
                set_error_message (CST21("Error in '\\1' at \\2:\\3:\r\nname '\\4' must be a direct string."), name);
                break;
            }
            name = expression->name;
            if(isnew && container_find(name))
            { set_error_message (CST21("Error in \\1 at \\2:\\3:\r\nMFETC name '\\4' already exists."), name); break; }
        }

        component = component_find(container, CST31("type"));
        if(component)
        {
            if(!component_parse(component, container)) break;

            expression = (component->root2==NULL) ? component->root1 : component->root2;
            if(expression==NULL || expression->evaluate != set_userstring)
            {
                name = (component->strExpr2==NULL) ? component->strExpr1 : component->strExpr2;
                set_error_message (CST21("Error in '\\1' at \\2:\\3:\r\ntype '\\4' must be a direct string."), name);
                break;
            }
        }

        success = true;
        if(*container_ptr==NULL)
        {  *container_ptr = container;
            DOUBLE_LINKED_LIST_NODE_ADD (head_container, container);
        }
        break;
    }
    if(!success && *container_ptr==NULL)
    {   dependence_finalise(false);
        container_remove(container);
    }
    return success;
}


/**************************************************************************************************************/

/* remove container from linked-list */
void container_remove (Container *container)
{
    if(container==NULL) return;
    DOUBLE_LINKED_LIST_NODE_REMOVE(head_container, container);
    container_destroy(container);
}

/* check for and return container if it exists */
bool container_check (const Container* container)
{
    const Container *c;
    if(container==NULL) return false;
    for(c = head_container; c != NULL; c = c->next)
        if(c == container) return true;
    sprintf2 (error_message, CST21("Container not in the list of existing containers.\r\n"));
    return false;
}

/* find and return a pointer to a container structure given its name */
Container *container_find (const lchar* name)
{
    Container *c;
    if(name==NULL) { sprintf2 (error_message, CST21("In container_find(): name==NULL\r\n")); return NULL; }

    for(c = head_container; c != NULL; c = c->next)
        if(0==strcmp33 (c->name, name))
            return c;
    sprintf2 (error_message, CST21("Cannot find MFET container with name '%s'."), CST23(name));
    return NULL;
}

/* find and return a pointer to a component structure given its name */
Component *component_find (Container* container, const lchar* name)
{
    char str[30];
    Component *c;
    if(container==NULL) return NULL;

    for(c = container->first; c != NULL; c = c->next)
    {
        if(c->state == DELETED) continue;
        if(c->cname2 == NULL)
            { if(0==strcmp33( (c->name2==NULL) ? c->name1 : c->name2 , name)) return c; }
        else // else component c returns a complex value structure
            { CNtoSTR(str, c); if(0==strcmp13 (str, name)) return c; }
    }
    sprintf2 (error_message, CST21("Cannot find variable with name '%s'."), CST23(name));
    return NULL;
}


/**************************************************************************************************************/


/* parse the string-expression of a component into an expression tree */
Component *component_parse (Component *component, Container *container)
{
    if(component==NULL) return NULL;

    if(component->state == DELETED)
    { strcpy21(error_message, "In component_parse() component->state==DELETED ?\n"); return NULL; }

    if(component->state == ISPARSE
    || component->state == NOPARSE
    || component->root2 != NULL) return component;

    component->root2 = parseExpression(
                            component->strExpr2,
                            component->parameter2,
                            component,
                            container);

    if(component->root2 == NULL) return NULL;
    return component;
}



/* evaluate the expression tree of a component, given its function arguments */
Value* component_evaluate (Component *component,
                           const Value* argument,   // in case of a function call.
                           const Value* result_vst) // 'expected' result structure.
{
    const lchar* name;
    Expression* expression;
    const Value* parameter;
    mchar *vst1, *vst2;

    if(component==NULL) return NULL;

    name = (component->root2==NULL) ? component->name1 : component->name2; // TODO: there is a bug
    parameter = (component->root2==NULL) ? component->parameter1 : component->parameter2;
    expression = (component->root2==NULL) ? component->root1 : component->root2;

    if(expression==NULL)
    { strcpy21(error_message, "Software Error in component_evaluate():\r\ncomponent->root1->root2==NULL."); return NULL; }

    if(argument && valueSt_compare(parameter, argument)&1) // if matching is not one-to-
    {
        vst1 = error_message + 500;
        vst2 = error_message + 750;
        VstToStr (argument , vst1, 1, -1, -1, -1);
        VstToStr (parameter, vst2, 1, -1, -1, -1);
        set_error_message (TWST(Parameter_Vst_Is_Invalid), name, vst1, vst2);
        puts2(error_message); return NULL;
    }

    Value* result = expression_evaluate(expression, argument);
    if(result==NULL) return NULL;

    if(result_vst && valueSt_compare(result, result_vst)) // if matching is not one-to-one
    {
        vst1 = error_message + 500;
        vst2 = error_message + 750;
        VstToStr (result_vst, vst1, 1, -1, -1, -1);
        VstToStr (result    , vst2, 1, -1, -1, -1);
        set_error_message (TWST(Result_Vst_Is_Invalid), name, vst1, vst2);
        puts2(error_message);
        value_free(result);
        return NULL;
    }
    return result;
}


/**************************************************************************************************************/


int evaluation_instance = 0;
Container* replacement_container = NULL;


/* Record a the replacement operator := */
void replacement_record (Container *c, const Expression *replace)
{
    int i;
    if(!c || !replace) return;
    c->replacement_exist = true;

    for(i=0; i < c->replacement_count; i++)
        if(c->replace[i] == replace) return;

    c->replace[i] = replace;
    c->replacement_count++;
}



/* Commit the replacement as previously recorded */
void replacement_commit (Container *c)
{
    mchar a;
    int i, j;
    mchar to[1000];
    const lchar* name;
    const Expression *expr;
    lchar *lstr, *start, *stop;
    if(c==NULL) return;
    if(c->text==NULL) return;

    for(i=0; i < c->replacement_count; i++)
    {
        // Step1: Locate where the replacement operator is
        name = c->replace[i]->name;
        if(name==NULL) { printf("Software Error in replacement_commit(): c->replace[%d]->name == NULL\n", i); continue; }
        start = NULL;
        stop = NULL;
        for(lstr = c->text; lstr->mchr != 0; lstr = lstr->next)
            if(lstr->line == name->line && lstr->coln == name->coln) break;

        if(lstr->mchr==0) { set_error_message (CST21("Software Error in '\\1' at \\2:\\3:\r\nOn '\\4': replacement_commit() is not done."), name); puts2(error_message); continue; }

        // Step2: Locate where start and stop both are
        for(j=1; lstr->prev != NULL; lstr = lstr->prev)
        {
            a = lstr->prev->mchr;
            if(isClosedBracket(a)) j++;
            if(isOpenedBracket(a)) j--;

            if(j<1) break;
            if(j==1 && (a==*TWSF(Assignment) ||  a==*TWSF(CommaSeparator))) break;

            if(!isSpace(a))
            {   start = lstr->prev;
                if(stop==NULL) stop = lstr->prev;
            }
        }
        if(start==NULL || stop==NULL) { set_error_message (CST21("Software Error in '\\1' at \\2:\\3:\r\nOn '\\4': replacement_commit() start=stop=NULL."), name); puts2(error_message); continue; }

        // Step3: Get the string to replace with
        expr = c->replace[i]->headChild;
        if(expr==NULL) { printf("Software Error in replacement_commit(): c->replace[%d]->headChild == NULL\n", i); continue; }
        VstToStr (expr->constant, to, 0, -1, -1, -1);
        lstr=NULL; astrcpy32(&lstr, to);

        if(lstr==NULL || lstr->mchr==0) { set_error_message (CST21("Software Error in '\\1' at \\2:\\3:\r\nOn '\\4': replacement_commit() has empty lstr."), name); puts2(error_message); continue; }
        //printf("replacement_commit():   container = '%s'   start = %d:%d   stop = %d:%d  to = '%s'\n", CST13(c->name), start->line, start->coln, stop->line, stop->coln, CST13(lstr));

        // Step4: Setup the start
        if(start->prev != NULL) start->prev->next = lstr;
        if(c->text == start) c->text = lstr;
        lstr->prev = start->prev;

        // Step5: Setup the stop
        for( ; lstr->next->mchr != 0; lstr = lstr->next);
        lchar_free (lstr->next); // free the end-of-string character

        stop->next->prev = lstr;
        lstr->next = stop->next;

        // Step6: Finalise
        lstr = NULL;
        start->prev = NULL;
        stop->next = NULL;
        lchar_free(start);
    }
    c->replacement_count=0;
    /*i = c->text->line;
    j = c->text->coln;
    if(i==0 || j==0) i=j=1;
    set_line_coln (c->text, i, j);*/
}

