/*
    structures.c
*/

#include <_stdio.h>
#include <component.h>

/***********************************************************************************************************/

static void expression_destroy (Expression *expression)
{
    if(expression==NULL) return;
    assert(expression->deleted==false);
    value v = expression->constant;
    if(isPoiter(v)) value_free(getPoiter(v));
  //if(isString(v)) lchar_free(getString(v));
    lchar_free (expression->name);
    memset(expression, 0, sizeof(Expression));
    expression->deleted = true; // see assert() above
    _free(expression);
    memory_freed("Expression");
}

void expression_remove (Expression *expression)
{
    Expression *expr, *expr_next;
    if(expression==NULL) return;
    assert(expression->deleted==false);

    // NOTE: inside set_current(Expression* current_type, ) we have:
    //  NOTE: using ->headChild makes expression_remove() to crash
    //  Expression *lhs = expression->lastChild; // left hand side

    // recursively remove children
    for(expr = expression->headChild; expr != NULL; expr = expr_next)
    {
        expr_next = expr->nextSibling;
        expression_remove(expr);
    }
    expression_destroy (expression);
}


static long expression_to_valueSt_2 (Expression *expr, value* vst)
{
    long c, len;

    if(expr->headChild==NULL)
    {
        len = 1;
        *vst = setString(expr->name);
        expr->name = NULL; // or do strcpy:
        //lstr=NULL;
        //if(expr->name!=NULL)
        //    astrcpy33(&lstr, expr->name);
        //*vst = setString(lstr);
    }
    else
    {   len = 1;
        for(c=0, expr = expr->headChild; expr != NULL; expr = expr->nextSibling, c++)
            len += expression_to_valueSt_2 (expr, vst+len);
        *vst = setSepto2(len, c);
    }
    return len;
}

value* expression_to_valueSt (Expression *expression)
{
    if(expression==NULL) return NULL;
    //expression_tree_print(expression); // in expresion.h

    value* vst = (value*)errorMessage();
    long len = expression_to_valueSt_2 (expression, vst);

    value* out = value_alloc (len);
    memcpy(out, vst, len*sizeof(value));

    //printf("expression_to_valueSt = '%s'\n", vst_to_str(vst));
    return out;
}

/***********************************************************************************************************/

static void inherits_clear (AVLT* tree)
{
    void* node = avl_min(tree);
    for( ; node != NULL; node = avl_next(node))
    {
        Component* c = *(Component**)node;
        if(c) c->type1 = NULL;
    }
    avl_free(tree);
}

void component_destroy (Component *component)
{
    if(component==NULL) return;
    memory_freed("Component");

    component_remove (&component->inners);
    inherits_clear (&component->inherits);

    depend_denotify (&component->depend1, component);
    list_free (&component->depend1);
    list_free (&component->depend2);
    list_free (&component->depOnMe);
    lchar_free (component->mfet1);
    lchar_free (component->mfet2);
    lchar_free (component->name1);
    lchar_free (component->name2);
    lchar_free (component->text1);
    lchar_free (component->text2);
    value_free (component->para1);
    value_free (component->para2);
    value_free (component->result1);
    value_free (component->result2);
    expression_remove (component->root1);
    expression_remove (component->root2);

    if(component->name1 && !component->isaf1)
    {   void* node = list_find(containers_list, NULL, pointer_compare, &component);
        assert(node!=NULL);
        list_delete(containers_list, node);
    }
    if(component->owner) *(Container**)component->owner = NULL;
    if(component->type1) avl_do(AVL_DEL, &component->type1->inherits, &component, 0,0,0);
    if(!component->name1 && !component->name2) avl_delete(NULL, component); // must come before
    if(component->parent) avl_delete(&component->parent->inners, component); // must be very last
}

void component_remove (AVLT* tree)
{
    Component *c = (Component*)avl_min(tree);
    for( ; c != NULL; c = (Component*)avl_next(c))
    {
        c->parent = NULL;
        component_destroy(c);
    }
    avl_free(tree);
}

/***********************************************************************************************************/

static void getindent (char* str, const char* text, int indent)
{
    char *s = str;
    int i;
    if(indent<0) indent=1;
    for(*s=0, i=0; i<indent; i++)
    { strcpy11(s, text); s += strlen1(s); }
}

void inherits_obtain (Component *component, List* list, wchar* mstr, const char* text, int indent)
{
    char str[1000];
    assert(component!=NULL);
    if(list) list_tail_push(list, list_new(&component, sizeof(component)));
    if(mstr)
    {
        getindent(str, text, indent);
        mstr = strcpy21(mstr, str);
        mstr = strcpy23(mstr, c_name(component));
        mstr = strcpy21(mstr, "\r\n");
    }
    if(indent>=-1)
    {
        if(indent<0) indent--; else indent++;
        void* node = avl_min(&component->inherits);
        for( ; node != NULL; node = avl_next(node))
        {
            inherits_obtain(*(Component**)node, list, mstr, text, indent);
            if(mstr) mstr += strlen2(mstr);
        }
    }
}

bool inherits_remove (Component *component)
{
    //if(!component) return true;
    assert(component!=NULL);
    List list={0};

    wchar* message = errorMessage();
    wchar* mstr = message;
    sprintf2(mstr, L"On removal of container:\r\n");

    mstr += strlen2(mstr);
    inherits_obtain(component, &list, mstr, "* ", 1);

    mstr += strlen2(mstr);
    strcpy21(mstr, "All will be removed.\r\n");

    bool remove = (list.size==1 || wait_for_confirmation (L"Confirm removal", message));
    if(remove)
    {   void* node = list_tail(&list);
        for( ; node!=NULL; node = list_prev(node))
            component_destroy(*(Component**)node);
    }
    list_free(&list);
    return remove;
}

/***********************************************************************************************************/

static bool depend_check (List depend)
{   void* node = list_head(&depend);
    for( ; node!=NULL; node = list_next(node))
        if(*(Component**)node==NULL) return 0;
    return 1;
}

static inline const char* state2str (enum STATE state)
{
    switch(state){
        case ISPARSE: return "ISPARSE";
        case DOPARSE: return "DOPARSE";
        case NOPARSE: return "NOPARSE";
        case DELETED: return "DELETED";
        case CREATED: return "CREATED";
        default: return "NULL";
    }
}


void expression_print (const char* text, int indent, const Expression *expr)
{
    if(!expr) return;
    char str[1000];
    getindent(str, text, indent);
    printf("\n");
    printf("%s expression      = %p\n"   , str, (void*)expr);
    printf("%s exr->name       = '%s'\n" , str, CST13(expr->name));
    printf("%s exr->line:coln  = %d:%d\n", str, expr->name->line, expr->name->coln);
    printf("%s exr->ID         = %d\n"   , str, expr->ID);
    printf("%s exr->type       = 0x%x\n" , str, expr->type);
    printf("%s exr->previous   = 0x%x\n" , str, expr->previous);
    printf("%s exr->precedence = %d\n"   , str, expr->precedence);
    printf("%s exr->constant   = '%s'\n" , str, vst_to_str(&expr->constant));
    printf("\n\n");
}


void component_print (const char* text, int indent, const Component *component)
{
    if(component==NULL) return;
    char str[1000];
    getindent(str, text, indent);
    int line, coln;

        printf("%s component            = %p\n", str, (void*)component);

    if(component->type1)
        printf("%s component->type1     = '%s'\n", str, component->type1 ? CST13(c_name(component->type1)) : "NULL");
    if(component->type2)
        printf("%s component->type2     = '%s'\n", str, component->type2 ? CST13(c_name(component->type2)) : "NULL");

    if(component->name1) { line = component->name1->line; coln = component->name1->coln;
        printf("%s component->name1     = '%s' %d:%d\n", str, CST13(component->name1), line, coln); }

    if(component->name2) { line = component->name2->line; coln = component->name2->coln;
        printf("%s component->name2     = '%s' %d:%d\n", str, CST13(component->name2), line, coln); }

    if(component->text1) { line = component->text1->line; coln = component->text1->coln;
        printf("%s component->text1     = '%s' %d:%d\n", str, CST13(component->text1), line, coln); }

    if(component->text2) { line = component->text2->line; coln = component->text2->coln;
        printf("%s component->text2     = '%s' %d:%d\n", str, CST13(component->text2), line, coln); }

    if(component->para1)
        printf("%s component->para1     = %p\n", str, (void*)component->para1);
    if(component->para2)
       {printf("%s component->para2     = %p\n", str, (void*)component->para2);}

        printf("%s component->isaf      = %s\n", str, c_isaf(component) ? "true" : "false");
        printf("%s component->access    = %s\n", str, access2str(c_access(component)));
        printf("%s component->state     = %s\n", str, state2str(component->state));
        printf("%s component->parent    = %p\n", str, (void*)component->parent);

    if(component->depOnMe.size)
        printf("%s component->depOnMe   = %ld  check=%d\n", str, component->depOnMe.size, depend_check(component->depOnMe));

    if(component->depend1.size)
        printf("%s component->depend1   = %ld  check=%d\n", str, component->depend1.size, depend_check(component->depend1));

    if(component->depend2.size)
        printf("%s component->depend2   = %ld  check=%d\n", str, component->depend2.size, depend_check(component->depend2));

    if(component->root1)
        printf("%s component->root1     = %p\n", str, (void*)component->root1);
    if(component->root2)
        printf("%s component->root2     = %p\n", str, (void*)component->root2);

    if(component->inherits.size)
        printf("%s component->inherits.size = %d\n", str, component->inherits.size);

    if(component->inners.size)
        printf("%s component->inners.size = %d\n", str, component->inners.size);

    printf("\n");

    if(indent>=-1)
    {
        if(indent<0) indent--; else indent++;
        void* c = avl_min((AVLT*)(size_t)&component->inners);
        for( ; c != NULL; c = avl_next(c))
            component_print(text, indent, (Component*)c);
    }
}

