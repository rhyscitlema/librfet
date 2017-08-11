/*
    component.c
*/

#include <_stdio.h>
#include <_texts.h>
#include <component.h>
#include <expression.h>
#include "operations.h"

static List _containers_list = {0};
List* containers_list = &_containers_list;

static Container *RootContainer = NULL;

int pointer_compare (const void* key1, const void* key2, const void* arg)
{
    const void* a = *(const void**)key1;
    const void* b = *(const void**)key2;
    return a<b ? -1 : a>b ? +1 : 0;
}

/**************************************************************************************************************/


/* read: I 'component', am depending on 'depending' */
void depend_add (Component* component, Component *depending)
{
    if(component==NULL) return;
    if(component==depending) return;
    if(list_find(&component->depend2, NULL, pointer_compare, &depending)) return;
    list_head_push(&component->depend2, list_new(&depending, sizeof(depending)));
}



/* read: I 'depend', am depended upon by 'component' */
static void depend_notify (List* depend, Component *component)
{
    void* node = list_head(depend);
    for( ; node != NULL; node = list_next(node))
    {
        Component* c = *(Component**)node;
        if(list_find(&c->depOnMe, NULL, pointer_compare, &component)) continue;
        list_head_push(&c->depOnMe, list_new(&component, sizeof(component)));
    }
}

/* read: I 'depend', am not depended upon by 'component' */
void depend_denotify (List* depend, const Component *component)
{
    void* node = list_head(depend);
    for( ; node != NULL; node = list_next(node))
    {
        Component* c = *(Component**)node;
        void* dep = list_find(&c->depOnMe, NULL, pointer_compare, &component);
        if(dep) list_delete(&c->depOnMe, dep);
    }
}


/**************************************************************************************************************/

// TODO: just like it happens during deletion, component_finalise() should be
// implemented such that the child containers are resolved before their parents.
// Otherwise something like the update_replace_count() below will have a bug.
// This bug is that the assert(c!=NULL) will fail in the following scenario:
// * Evaluate: "0; type="User_Interface_Definition_Text"; replace h1 = 160;"
// * then later evaluate: "0;", that is removing both 'type' and 'replace'.

static inline void update_replace_count (Component* component, int offset)
{
    Component *c = component_find(component->parent, c_name(component), NULL, true);
    if(c==NULL) return; //assert(c!=NULL);
    c->replace += offset; assert(c->replace>=0);
}

static AVLT dependence = { 0, 0, sizeof(Component*), NULL, pointer_compare };

static void component_finalise (Component *component, bool success)
{
    if(component==NULL) return;
    if(success)
    {
        if(component->state==DELETED)
        {
            if(component->access1 == ACCESS_REPLACE) update_replace_count(component, -1);
            // line above must come before the component destruction below

            // this section is quite tricky, but the 'while' line below tells alot!
            if(component->inners.size==0)
            {   Container* parent;
                do{ parent = component->parent;
                    component_destroy(component);
                    component = parent;
                } while(parent->state==DELETED && parent->inners.size==0);
            }
        }
        else
        {
            if(component->state==CREATED && !component->isaf2) list_head_push(containers_list, list_new(&component, sizeof(component)));

            if(component->access1 != ACCESS_REPLACE && component->access2 == ACCESS_REPLACE) update_replace_count(component, +1);
            if(component->access1 == ACCESS_REPLACE && component->access2 != ACCESS_REPLACE) update_replace_count(component, -1);

            lchar_free(component->name1); component->name1 = component->name2; component->name2 = NULL;
            lchar_free(component->text1); component->text1 = component->text2; component->text2 = NULL;
            value_free(component->para1); component->para1 = component->para2; component->para2 = NULL;
            lchar_free(component->rfet1); component->rfet1 = component->rfet2; component->rfet2 = NULL;

            component->isaf1 = component->isaf2;
            component->access1 = component->access2;
            depend_denotify(&component->depend1, component);
            depend_notify  (&component->depend2, component);
            list_free(&component->depend1); component->depend1 = component->depend2; list_clear(&component->depend2);
            if(component->root2 != NULL) { expression_remove(component->root1); component->root1 = component->root2; component->root2 = NULL; }
            if(component->isaf1) component_remove(&component->inners); // if container to function conversion

            if(component->type1 != component->type2) {
                if(component->type1) avl_do(AVL_DEL, &component->type1->inherits, &component, 0,0,0);
                if(component->type2) avl_do(AVL_ADD, &component->type2->inherits, &component, 0,0,0);
            } component->type1 = component->type2; component->type2 = NULL;

            component->state = ISPARSE;
        }
    }
    else
    {   if(component->state==CREATED)
        {
            // this section is quite tricky, but the 'while' line below tells alot!
            if(component->inners.size==0)
            {   Container* parent;
                do{ parent = component->parent;
                    component_destroy(component);
                    component = parent;
                } while(parent->state==DELETED && parent->inners.size==0);
            }
            else component->state = DELETED;
        }
        else
        {
            lchar_free(component->name2); component->name2 = NULL;
            lchar_free(component->text2); component->text2 = NULL;
            value_free(component->para2); component->para2 = NULL;
            lchar_free(component->rfet2); component->rfet2 = NULL;

            component->isaf2 = component->isaf1;
            component->access2 = component->access1;
            list_free(&component->depend2);
            expression_remove(component->root2); component->root2 = NULL;
            component->type2 = NULL;

            component->state = ISPARSE;
        }
    }
}

static void dependence_notify (Component *component)
{
    if(component==NULL) return;
    if(!avl_do(AVL_ADD, &dependence, &component, 0,0,0)) memory_alloc("Dependence");

    if(!component->isaf1) return; // if is a container TODO: eh, so what if is a container?

    if(component->state == NOPARSE) component->state = DOPARSE; // TODO: consider commenting this line

    if(component->state != ISPARSE) return;
    component->state = DOPARSE;

    void* node = list_head(&component->depOnMe);
    for( ; node != NULL; node = list_next(node))
        dependence_notify(*(Component**)node);
}

/*bool dependence_evaluate ()
{
    void* ptr = avl_min(&dependence);
    for( ; ptr!=NULL; ptr = avl_next(ptr))
        if(!component_evaluate(*(Component**)ptr,0,0))
            return false;
    return true;
}*/

bool dependence_parse ()
{
    void* ptr = avl_min(&dependence);
    for( ; ptr!=NULL; ptr = avl_next(ptr))
        if(!component_parse(*(Component**)ptr))
            return false;
    return true;
}

void dependence_finalise (bool success)
{
    void* ptr = avl_min(&dependence);
    for( ; ptr!=NULL; ptr = avl_next(ptr))
    {
        component_finalise(*(Component**)ptr, success);
        memory_freed("Dependence");
    }
    avl_free(&dependence);
}


/**************************************************************************************************************/

typedef struct _InnerFunction {
    lchar* name;
    lchar* text;
    value* para;
    enum ACCESS access;
} InnerFunction;

typedef struct _InnerContainer {
    lchar* rfet;
    enum ACCESS access;
} InnerContainer;

static bool component_extract (const lchar* input, List* innerFunctions, List* innerContainers, wchar* errormessage)
{
    int size, brackets;
    const Expression *nextItem;
    Expression* expression;
    const lchar* start;
    lchar* str = NULL;
    lchar* name = NULL;
    value* para = NULL;
    enum ACCESS access;
    bool error=false;
    bool is_main_comp = true;

    InnerFunction  ifunc;
    InnerContainer icont;
    assert(innerFunctions!=NULL && innerContainers!=NULL);

    if(isEmpty3(input,0)) { strcpy21(errormessage, "Empty input."); return false; }

    while(input->wchr!=0) // while getting components
    {
        /* Step0: initialise */
        lchar_free(name);  name = NULL;
        value_free(para);  para = NULL;
        access = ACCESS_PROTECTED;

        if(!is_main_comp)
        {
            /* Step1: Obtain component access control */
            nextItem = get_next_item (&input, &str, 0,0);
            if(nextItem==NULL || input->wchr==0) break;

            bool b=0;
            if(0==strcmp31(str,"private"  )) { b=1; access = ACCESS_PRIVATE; }
            if(0==strcmp31(str,"enclosed" )) { b=1; access = ACCESS_ENCLOSED; }
            if(0==strcmp31(str,"protected")) { b=1; access = ACCESS_PROTECTED; }
            if(0==strcmp31(str,"public"   )) { b=1; access = ACCESS_PUBLIC; }
            if(0==strcmp31(str,"replace"  )) { b=1; access = ACCESS_REPLACE; }

            /* Step2: Obtain the component */
            if(b)
            {   nextItem = get_next_item (&input, &str, 0,0);
                if(nextItem==NULL || input->wchr==0) break;
            }

            /* Step2.0: Obtain component as an inner-container */
            if(0==strcmp31(str,"\\rfet"))
            {
                if(input->wchr != '{')
                { set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nExpected '{' directly after \\rfet.", input); error=true; break; }

                size = 1;
                start = input = input->next; // skip '{'
                while(true)
                {
                    input = lchar_next(input);
                    if(input->wchr=='{') size++;
                    if(input->wchr=='}') size--;
                    if(input->wchr==0)
                    { set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nExpected a closing '}'.", input); error=true; break; }
                    if(size==0) break;
                }
                if(size!=0) break;

                size = strlen3S(start, input);
                astrcpy33S (&str, start, size);
                input = input->next; // skip '}'

                icont.rfet = str; str=NULL;
                icont.access = access;
                list_tail_push(innerContainers, list_new(&icont, sizeof(icont)));
                continue;
            }

            if(str->wchr=='\\')
            { set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nOn '\\4': expected \\rfet{ only.", str); error=true; break; }

            if(!(nextItem->type & AVARIABLE))
            { set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nOn '\\4': expected a component name.", str); error=true; break; }

            else { name = str; str=NULL; }

            nextItem = get_next_item (&input, &str, 0,0);
            if(nextItem==NULL || input->wchr==0) break;
        }

        if(is_main_comp && input->wchr=='\\' && input->next->wchr=='(')
        {
            is_main_comp = false;
            input = input->next;
            nextItem = get_next_item (&input, &str, 0,0);
            if(nextItem==NULL || input->wchr==0) break;
        }

        if(!is_main_comp)
        {
            /* Step3: Obtain component parameter */
            if(str->wchr == *TWSF(Opened_Bracket_1))
            {
                // if '(' then obtain parameter structure
                bool empty=true;
                brackets=1;
                start=input->prev;
                while(input->wchr!=0)
                {
                    if(input->wchr == *TWSF(Opened_Bracket_1)) brackets++;
                    if(input->wchr == *TWSF(Closed_Bracket_1)) brackets--;
                    input = lchar_next(input);
                    if(brackets==0) break;
                    if(!isSpace(input->wchr)) empty=false;
                }
                if(!empty)
                {
                    size = strlen3S(start, input);
                    astrcpy33S (&str, start, size);

                    expression = parseExpression (str,0);
                    if(expression==NULL) { error=true; break; }

                    para = expression_to_valueSt (expression);
                    expression_remove(expression); expression=NULL;
                }
                nextItem = get_next_item (&input, &str, 0,0);
                if(nextItem==NULL || input->wchr==0) break;
            }

            /* Step 4: final check for a '=' assignment operator */
            if(str->wchr != *TWSF(Assignment))
            { set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nOn '\\4': expected assignment '=' instead.", str); error=true; break; }
        }
        else is_main_comp = false;

        /* Step 5: collect the Math Function Expression Text */
        start=input;
        size=0;
        while(input->wchr != 0 && input->wchr != *TWSF(EndOfStatement))
            input = lchar_next(input);
        size = strlen3S(start, input);
        if(input->wchr != 0) input = input->next; // skip EndOfStatement
        astrcpy33S (&str, start, size?size:1); // str = RFET
        if(size==0) str->wchr=0;

        /* Step 7: iterative tree-traversal to record sub-components of a component's result-structure */
        /*if(VST_LEN(cname) > 1)
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
                          { lchar_copy(name, getString(*cname));
                            if(name->wchr==0) break;
                            name = name->next;
                          } name=NULL;
                        }
                    astrcpy33 (&name, getString(*cname));
                    avaluecpy (&parameter, tcomponent->parameter2);

        // Step 8: collect the Math Function Expression Text //

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
        }*/

        ifunc.name = name; name=NULL;
        ifunc.text = str; str=NULL;
        ifunc.para = para; para=NULL;
        ifunc.access = access;
        list_tail_push(innerFunctions, list_new(&ifunc, sizeof(ifunc)));
    }

    lchar_free(str);
    lchar_free(name);
    value_free(para);
    return !error;
}



static int avl_component_cmpr (const void* key1, const void* key2, const void* arg)
{ return strcmp33( c_name((const Component*)key1), c_name((const Component*)key2) ); }

// note: after call, set name to NULL
static Component* component_new (Container* parent, lchar* name, bool isaf)
{
    memory_alloc("Component");
    Component* component = (Component*)avl_new(NULL, sizeof(Component));
    component->parent = parent;
    component->state = CREATED;
    component->isaf2 = isaf;
    component->name2 = name;
    component->inherits.keysize = sizeof(Component*);
    component->inherits.compare = pointer_compare;
    if(parent) avl_do(AVL_PUT, &parent->inners, component, sizeof(Component), NULL, avl_component_cmpr);
    return component;
}



static bool component_insert (Container* container, List* innerFunctions, List* innerContainers, wchar* errormessage)
{
    InnerFunction*  ifunc = (InnerFunction *)list_head(innerFunctions );
    InnerContainer* icont = (InnerContainer*)list_head(innerContainers);

    Component* component;
    lchar* name  = NULL;
    lchar* text  = NULL;
    value* para  = NULL;
    enum ACCESS access;

    bool error = container==NULL;
    if(!error)
    {   component = (Component*)avl_min(&container->inners);
        for( ; component != NULL; component = (Component*)avl_next(component))
        {   if(component->state!=ISPARSE) component_print("component_insert ", -1, component);
            assert(component->state==ISPARSE);
        }
    }
    while(ifunc != NULL && !error)
    {
        name  = ifunc->name;
        text  = ifunc->text;
        para  = ifunc->para;
        access= ifunc->access;
        ifunc = (InnerFunction*)list_next(ifunc);

        if(!name) component = container; // if is_main_comp
        else
        {
            // set to private irrespective of user-defined access type
            if(0==strcmp31(name,"name")) access = ACCESS_PRIVATE;

            /* Step 6: record the extracted component */
            component = component_find (container, name, errormessage, 0);
            int i = component==NULL ? 0                 // if not already defined
                  : component->parent!=container ? 1    // if inherited from a parent-container
                  : 2 ;
            if(i==1 && c_access(component) > access)
            {
                strcpy21(ErrStr0, access2str(c_access(component)));
                strcpy21(ErrStr1, access2str(access));
                set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\nCannot downgrade access type: \\5 to \\6.", name, ErrStr0, ErrStr1);
                error=true; break;
            }
            if(i!=2) { component = component_new(container, name, true); name=NULL; }

            else if(component->name2!=NULL) // if already exists
            { set_message(errormessage, TWST(Component_Already_Defined), name); error=true; break; }

            else if(0!=strcmp33( c_text(component), text)
                 || 0!=value_compare( c_para(component), para))
                 component->state = DOPARSE;
            else component->state = NOPARSE;
        }

        if(name) {
        assert(component->name2==NULL); component->name2 = name; name=NULL; }
        assert(component->text2==NULL); component->text2 = text; text=NULL;
        assert(component->para2==NULL); component->para2 = para; para=NULL;
        component->access2 = access;
        dependence_notify(component);

        if(access==ACCESS_REPLACE)
        {   Component* c = component_find(container, c_name(component), errormessage, true);
            if(c==NULL) { error=true; break; }
            depend_add(component, c);
        }
    }
    while(icont != NULL && !error)
    {
        access = icont->access;
        Container *cont = container_parse(container, NULL, icont->rfet);
        icont  = (InnerContainer*)list_next(icont);
        if(!cont) { error=true; break; }
        else cont->access2 = access;
    }

    while(ifunc != NULL)
    {
        lchar_free(ifunc->name);
        lchar_free(ifunc->text);
        value_free(ifunc->para);
        ifunc = (InnerFunction*)list_next(ifunc);
    }
    while(icont != NULL)
    {
        lchar_free(icont->rfet);
        icont = (InnerContainer*)list_next(icont);
    }
    if(!error)
    {   component = (Component*)avl_min(&container->inners);
        for( ; component != NULL; component = (Component*)avl_next(component))
            if(component->state==ISPARSE)
            {  component->state = DELETED;
               dependence_notify(component);
            }
    }
    lchar_free(name);
    lchar_free(text);
    value_free(para);
    list_free(innerFunctions);
    list_free(innerContainers);
    return !error;
}


static bool get_string_expr (const lchar* lstr, lchar** name_ptr, wchar* errormessage)
{
    const Expression* expression = get_next_item (&lstr, name_ptr, 0,0);
    const lchar* name = *name_ptr;
    if(expression==NULL || expression->ID != SET_STRING)
    { set_message(errormessage, L"Error in '\\1' at \\2:\\3:\r\nname '\\4' must be a direct string.", name); return false; }
    for(lstr=name; lstr->wchr!=0 && lstr->wchr!='|'; lstr = lstr->next);
    if(lstr->wchr=='|' || 0==strcmp31(name, ".") || 0==strcmp31(name, ".."))
    { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\nMust not contain '|' and must not be '.' nor '..'.", name); return false; }
    return true;
}


Container* container_parse (Container* parent, lchar* name, lchar* text)
{
    List _innerFunctions  = {0}, *innerFunctions  = &_innerFunctions;
    List _innerContainers = {0}, *innerContainers = &_innerContainers;

    if(!RootContainer) RootContainer = component_new(NULL, NULL, false);
    if(!parent) parent = RootContainer;
    Container* container=NULL;

    bool skip = false;
    bool found = false;
    bool success = false;
    const lchar* lstr=NULL;
    const lchar* type=NULL;
    wchar* errormessage = errorMessage();

    while(true) // not a loop
    {
        if(text)
        {
            if(!component_extract (text, innerFunctions, innerContainers, errormessage)) break;

            InnerFunction*  ifunc = (InnerFunction*)list_head(innerFunctions);
            for( ; ifunc != NULL; ifunc = (InnerFunction*)list_next(ifunc))
            {
                if(0==strcmp31(ifunc->name, "name")) lstr = ifunc->text;
                if(0==strcmp31(ifunc->name, "type")) type = ifunc->text;
            }
            if(lstr!=NULL) { if(get_string_expr(lstr, &name, errormessage)) found=true; else break; }
        }
        else assert(name!=NULL);

        if(name)
        {   Container cont={0}; cont.parent = parent;
            container = container_find(&cont, name, errormessage, 0,0);
            if(container && container->parent != parent) { skip=true; container=NULL; }
        }
        if(text==NULL) // then name = "file name"
        {
            if(name->wchr!='|') break;  // File name must start with '|'
            text=name; name=NULL; astrcpy33(&name, text->next); // skip '|'

            if(container) { success=true; break; }

            Array2 wtext={0};
            if((wtext = FileOpen2(CST23(name), wtext)).size<=0) break;
            // do not set error message, use that from container_find()

            astrcpy32(&text, wtext.data);
            wchar_free(wtext.data);

            set_line_coln_source(text, 1, 1, CST23(name));

            if(!component_extract (text, innerFunctions, innerContainers, errormessage)) break;
        }

        if(!found && parent!=RootContainer) { set_message(errormessage, L"Error in \\1 at \\2:\\3:\r\nNon-top level container must have a name.", text); break; }

        if(!container)
        {
            if(!name) assert(parent != RootContainer); // if assert is true then errormessage above will be executed, since found=true means name!=NULL.
            else if(checkfail(parent, name)) break;
            container = component_new(parent, name, false); name=NULL;
        }
        else if(checkfail(container, NULL)) break;
        else
        {   container->state = DOPARSE;
            container->name2 = name; name=NULL;
        }

        if(type)
        {
            if(!get_string_expr(type, &name, errormessage)) break;
            type = name;

            bool same = 0==strcmp33(type, container->name2);
            if(!skip) skip = same;
            else if(!same)
            { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\nThe type must be the same as the name so to override.", type); break; }

            Container *cont_type = container_find(container, type, 0, skip, 0);
            assert(cont_type != container);
            if(!cont_type) { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\nSibling container does not exist.", type); break; }

            if(c_para(cont_type)!=NULL)
            { set_message(errormessage, L"Error in \\1 at \\2:\\3 on '\\4':\r\nCannot inherit a sibling that takes parameters.", type); break; }

            else container->type2 = cont_type;
        }

        container->rfet2 = text; text=NULL;
        success = true;
        break;
    }
    if(container) { if(!avl_do(AVL_ADD, &dependence, &container, 0,0,0)) memory_alloc("Dependence"); }

    // note: this call must always be done so to free innerFunctions and innerContainers
    if(!component_insert(success?container:NULL, innerFunctions, innerContainers, errormessage)) container=NULL;

    if(container && parent==RootContainer) container->access2 = ACCESS_PUBLIC;

    lchar_free(name);
    lchar_free(text);
    return container;
}


/**************************************************************************************************************/


static int avl_component_find (const void* key1, const void* key2, const void* arg)
{ return strcmp33( (const lchar*)key1, c_name((const Component*)key2) ); }

/* find and return a pointer to a container structure given its pathname */
Container *container_find (Container* current, const lchar* pathname, wchar* errormessage, bool skipFirst, bool fullAccess)
{
    if(!RootContainer) RootContainer = component_new(NULL, NULL, false);
    if(!pathname) return RootContainer;
    if(!current) current = RootContainer; // if this then pathname must start with '|'

    lchar* name=NULL;
    int access = ACCESS_PRIVATE;
    bool firsttime = true;
    for( ; ; firsttime=false)
    {
        if(pathname->wchr==0) break;
        const lchar* start = pathname;
        while(pathname->wchr != '|' && pathname->wchr != 0) pathname = pathname->next;
        astrcpy33S(&name, start, strlen3S(start, pathname));
        if(pathname->wchr!=0) pathname = pathname->next; // skip the '|'

             if(name == NULL) { if(firsttime) { current = RootContainer; access = ACCESS_PUBLIC; } continue; }
        else if(0==strcmp31(name, "." )) { continue; }
        else if(0==strcmp31(name, "..")) { if(!(current = current->parent)) current = RootContainer; continue; }
        else if(firsttime) current = current->parent; // default goes to parent

        if(0==strcmp31(name,"main")) break;
        Container *from = current;
        Container *c=NULL;
        while(true)
        {
            if(skipFirst) skipFirst=false;
            else
            {
                c = (Container*) avl_do (AVL_FIND, &current->inners, name, 0, NULL, avl_component_find);
                if(c!=NULL && c->state != DELETED) // TODO: instead do assert(c->state != DELETED);
                {
                    enum ACCESS acc = c_access(c);
                    if(!fullAccess && acc < access)
                    {
                        if(errormessage)
                        {   sprintf1((char*)ErrStr0,
                                    "Error in \\1 at \\2:\\3 on '\\4':\r\n"
                                    "component has %s access inside '\\5',\r\n"
                                    "expected at least %s access.",
                                    access2str(acc), access2str(access));
                            set_message(errormessage, CST21((char*)ErrStr0), name, CST23(c_name(current)));
                        }
                        c=NULL;
                    }
                    break;
                }
            }
            c = current;
            current = c_type(current);
            if(current==NULL)
            {
                if(errormessage)
                {
                    strcpy21(ErrStr0, "Error in \\1 at \\2:\\3:\r\nCannot find component '\\4' from \\5.");
                    const wchar* fr = (from==RootContainer) ? L"|" : CST23(c_name(from));
                    set_message(errormessage, ErrStr0, name, fr);
                }
                c=NULL; break;
            }
            const Container *p1 = current->parent, *p2 = c->parent;
            if(p1 == p2) // if have same parent
            { if(access == ACCESS_PRIVATE) access = ACCESS_ENCLOSED; }
            else if(p1->parent == p2->parent) // if have same grandpa
            { if(access <= ACCESS_ENCLOSED) access = ACCESS_PROTECTED; }
            else access = ACCESS_PUBLIC;
        }
        current = c;
        if(c==NULL) break;
        if(access < ACCESS_PUBLIC) access++;
    }
    lchar_free(name);
    return current;
}


/* find and return a pointer to a component structure given its name */
Component *component_find (Container* current, const lchar* name, wchar* errormessage, bool skipFirst)
{
    if(current==NULL) return NULL;
    if(name==NULL) return NULL;
    int len = strlen3(name);
    lchar pathname[MAX_PATH_SIZE];
    set_lchar_array(pathname, 2+len+1);
    strcpy33(pathname+2, name);
    lchar_copy(pathname+0, name); pathname[0].wchr = '.';
    lchar_copy(pathname+1, name); pathname[1].wchr = '|';
    return container_find (current, pathname, errormessage, skipFirst, false);
}


/* build and return the full path of given container */
const wchar* container_path_name (const Container* container)
{
    static wchar path[MAX_PATH_SIZE];
    if(!container) container = RootContainer;

    List list={0};
    for( ; container!=RootContainer; container = container->parent)
        list_head_push(&list, list_new(&container, sizeof(container)));

    wchar* mstr = path;
    void* node = list_head(&list);
    for( ; node != NULL; node = list_next(node))
    {
        container = *(Container**)node;
        *mstr++ = '|';
        strcpy23(mstr, c_name(container));
        mstr += strlen2(mstr);
    }
    *mstr=0;
    list_free(&list);
    return path;
}


/**************************************************************************************************************/


/* parse the string-expression of a component into an expression tree */
Component *component_parse (Component *component)
{
    if(component==NULL) return NULL;

    if(component->state == ISPARSE
    || component->state == NOPARSE
    || component->state == DELETED
    || component->root2 != NULL) return component;

    if(c_access(component)==ACCESS_REPLACE
    && !component_find(component->parent, c_name(component), errorMessage(), true)) return NULL;

    if(component->name2==NULL)
    {   astrcpy33(&component->name2, component->name1);
        astrcpy33(&component->text2, component->text1);
        avaluecpy(&component->para2, component->para1);
    }
    component->root2 = (Expression*)1; // so to deal with recursive call
    component->root2 = parseExpression(component->text2, component);

    if(component->root2 == NULL) return NULL;
    return component;
}



/* evaluate the expression tree of a component */
bool component_evaluate (ExprCallArg eca, Component *component,
                         const value* result_vst)  // 'expected' result structure.
{
    if(component==NULL) return false;
    if(component->state==DELETED) return true;
    const value* argument = eca.garg->argument;
    wchar* errormessage = eca.garg->message;

    bool success = false;
    while(true) // not a loop
    {
        const lchar* name = c_name(component);
        const value* para = c_para(component);
        Expression * expr = c_root(component);
        assert(expr!=NULL);

        if(argument && valueSt_compare(para, argument)>3) // if matching is not valid
        {
            VstToStr (argument, ErrStr0, 1, -1, -1, -1);
            VstToStr (para    , ErrStr1, 1, -1, -1, -1);
            set_message(errormessage, TWST(Parameter_Vst_Is_Invalid), name, ErrStr0, ErrStr1);
            break;
        }

        eca.expression = expr;
        if(!expression_evaluate(eca)) break;

        if(result_vst && valueSt_compare(eca.stack, result_vst)) // if matching is not one-to-one
        {
            VstToStr (result_vst, ErrStr0, 1, -1, -1, -1);
            VstToStr (eca.stack , ErrStr1, 1, -1, -1, -1);
            set_message(errormessage, TWST(Result_Vst_Is_Invalid), name, ErrStr0, ErrStr1);
            break;
        }
        success = true;
        break;
    }
    return success;
}


/**************************************************************************************************************/


int evaluation_instance = 0;
Container* replacement_container = NULL;


/* Record a the replacement operator := */
void replacement_record (Container *c, const Expression *replacement)
{
    if(!c || !replacement) return;
    int i;
    for(i=0; i < c->replacement_count; i++)
        if(c->replacement[i] == replacement) return;

    c->replacement[i] = replacement;
    c->replacement_count++;
}



/* Commit the replacement as previously recorded */
void replacement_commit (Container *c)
{
    wchar a;
    int i, j;
    wchar to[1000];
    const lchar* name;
    const Expression *expr;
    lchar *lstr, *start, *stop;

    if(c==NULL) return;
    if(!list_find(containers_list, NULL, pointer_compare, &c)) return; // TODO: try again to find the bug for when this line is removed
    assert(c->rfet1!=NULL);
    wchar* errormessage = errorMessage();

    for(i=0; i < c->replacement_count; i++)
    {
        // Step1: Locate where the replacement operator is
        name = c->replacement[i]->name;
        if(name==NULL) { printf("Software Error in replacement_commit(): c->replacement[%d]->name == NULL\n", i); continue; }
        start = NULL;
        stop = NULL;
        for(lstr = c->rfet1; lstr->wchr != 0; lstr = lstr->next)
            if(lstr->line == name->line && lstr->coln == name->coln) break;

        if(lstr->wchr==0) { set_message(errormessage, L"Software Error in '\\1' at \\2:\\3:\r\nOn '\\4': replacement_commit() is not done.", name); puts2(errormessage); continue; }

        // Step2: Locate where start and stop both are
        for(j=1; lstr->prev != NULL; lstr = lstr->prev)
        {
            a = lstr->prev->wchr;
            if(isClosedBracket(a)) j++;
            if(isOpenedBracket(a)) j--;

            if(j<1) break;
            if(j==1 && (a==*TWSF(Assignment) ||  a==*TWSF(CommaSeparator))) break;

            if(!isSpace(a))
            {   start = lstr->prev;
                if(stop==NULL) stop = lstr->prev;
            }
        }
        if(start==NULL || stop==NULL) { set_message(errormessage, L"Software Error in '\\1' at \\2:\\3:\r\nOn '\\4': replacement_commit() start=stop=NULL.", name); puts2(errormessage); continue; }

        // Step3: Get the string to replace with
        expr = c->replacement[i]->headChild;
        if(expr==NULL) { printf("Software Error in replacement_commit(): c->replacement[%d]->headChild == NULL\n", i); continue; }
        VstToStr (&expr->constant, to, 0, -1, -1, -1);
        lstr=NULL; astrcpy32(&lstr, to);

        if(lstr==NULL || lstr->wchr==0) { set_message(errormessage, L"Software Error in '\\1' at \\2:\\3:\r\nOn '\\4': replacement_commit() has empty lstr.", name); puts2(errormessage); continue; }
        //printf("replacement_commit():   container = '%s'   start = %d:%d   stop = %d:%d  to = '%s'\n", CST13(c->name), start->line, start->coln, stop->line, stop->coln, CST13(lstr));

        // Step4: Setup the start
        if(start->prev != NULL) start->prev->next = lstr;
        if(c->rfet1 == start) c->rfet1 = lstr;
        lstr->prev = start->prev;

        // Step5: Setup the stop
        for( ; lstr->next->wchr != 0; lstr = lstr->next);
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
}


/**************************************************************************************************************/


// main global variables
static int _stackSize = 0;
static value* _mainStack = NULL;
static wchar* _errorMessage = NULL;

int stackSize() { return _stackSize; }
value* mainStack() { return _mainStack; }
wchar* errorMessage() { return _errorMessage; }

void rfet_init (int stack_size)
{
    if(stack_size < 10000) stack_size = 10000;

    _stackSize = stack_size;
    _mainStack = (value*) _realloc (_mainStack, 2*stack_size*sizeof(value));
    _errorMessage = (wchar*) (_mainStack + stack_size);

    operations_init();
    SET_VSTXX();
}

void rfet_clean()
{
    // the order below matters
    component_destroy(RootContainer);
    RootContainer = NULL;
    _free(_mainStack);
    _mainStack=NULL;
    CST_clean();
    string_c_clean();
}

