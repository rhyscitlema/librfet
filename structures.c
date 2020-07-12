/*
	structures.c
*/

#include <stdio.h>
#include <_stdio.h>
#include <component.h>


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

static void component_remove (AVLT* tree)
{
	Component *c = (Component*)avl_min(tree);
	for( ; c != NULL; c = (Component*)avl_next(c))
	{
		c->parent = NULL;
		component_destroy(c);
	}
	avl_free(tree);
}

void component_destroy (Component *component)
{
	if(component==NULL) return;
	memory_freed("Component");

	component_remove (&component->inners);
	inherits_clear (&component->inherits);

	void* node = avl_min(&component->depOnMe);
	for( ; node != NULL; node = avl_next(node))
	{
		Component* c = *(Component**)node;
		avl_do(AVL_DEL, &c->depend1, &component, 0, 0, pointer_compare);
	}
	depend_denotify (&component->depend1, component);

	avl_free (&component->depend1);
	avl_free (&component->depend2);
	avl_free (&component->depOnMe);
	avl_free (&component->replacement);

	if(component->rfet1.ptr) // if component is a container
	{
		void* node = list_find(container_list(), NULL, pointer_compare, &component);
		assert(node!=NULL); list_delete(container_list(), node);
	}
	str3_free (component->name1);
	str3_free (component->name2);
	str3_free (component->text1);
	str3_free (component->text2);
	str3_free (component->rfet1);
	str3_free (component->rfet2);
	value_free(component->oper1);
	value_free(component->oper2);

	// see operations_evaluate() inside operations.c
	if(component->constant != 1+component->para1)
		value_free(component->constant);

	Container** owner = (Container**)component->owner;
	if(owner && *owner==component) *owner = NULL; // NOTE: do not assert anything

	if(component->type1) avl_do(AVL_DEL, &component->type1->inherits, &component, 0,0,0);
	if(component->parent) avl_delete(&component->parent->inners, component);
	// NOTE: last line above must really be last; also see component_remove().
}

/***********************************************************************************************************/

static void getindent (char* str, const char* text, int indent)
{
	int i, j;
	if(indent<0) indent=1;
	for(i=0; i<indent; i++)
		for(j=0; text[j]; j++)
			*str++ = text[j];
	*str=0;
}

Str2 inherits_obtain (Component *component, List* list, Str2 out, const char* text, int indent)
{
	char str[1000];
	assert(component!=NULL);
	if(list) list_tail_push(list, list_new(&component, sizeof(component)));

	getindent(str, text, indent);
	out = strcpy21(out, str);
	out = strcpy23(out, c_name(component));
	out = strcpy21(out, "\r\n");

	if(indent>=-1)
	{
		if(indent<0) indent--; else indent++;
		void* node = avl_min(&component->inherits);
		for( ; node != NULL; node = avl_next(node))
			out = inherits_obtain(*(Component**)node, list, out, text, indent);
	}
	return out;
}

bool inherits_remove (Component *component)
{
	assert(component!=NULL);
	if(component==NULL) return true;
	List list={0};

	wchar message[10000];
	Str2 str = message;
	str = strcpy21(str, "On removal of container:\r\n");

	str = inherits_obtain(component, &list, str, "* ", 1);

	str = strcpy21(str, "All will be removed.\r\n");

	bool remove = list.size==1 || user_confirm(L"Confirm removal", message);
	if(remove)
	{
		void* node = list_tail(&list);
		for( ; node!=NULL; node = list_prev(node))
			component_destroy(*(Component**)node);
	}
	list_free(&list);
	return remove;
}

/***********************************************************************************************************/

static inline const char* state2str (enum COMP_STATE state)
{
	switch(state){
		case ISPARSE: return "ISPARSE";
		case DOPARSE: return "DOPARSE";
		case NOPARSE: return "NOPARSE";
		case ISFOUND: return "ISFOUND";
		case DOFOUND: return "DOFOUND";
		case NOFOUND: return "NOFOUND";
		case CREATED: return "CREATED";
		default: return "NULL_STATE";
	}
}


static void do_print (const char* text, const char* name, const_Str3 str)
{
	if(strEnd3(str)) return;
	int line = sLine(str);
	int coln = sColn(str);
	printf("%s component->%s     = '%s' %d:%d %s\n",
		text, name, C13(str), line, coln, (str.end?"":"!str.end"));
}

void component_print (const char* text, int indent, const Component *component)
{
	if(component==NULL) return;
	char str[1000];
	getindent(str, text, indent);

		printf("%s component            = %p\n", str, (void*)component);

	if(component->type1)
		printf("%s component->type1     = '%s'\n", str, component->type1 ? C13(c_name(component->type1)) : "NULL");
	if(component->type2)
		printf("%s component->type2     = '%s'\n", str, component->type2 ? C13(c_name(component->type2)) : "NULL");

	do_print(str, "name1", component->name1);
	do_print(str, "name2", component->name2);
	do_print(str, "text1", component->text1);
	do_print(str, "text2", component->text2);

	if(component->rfet1.ptr)
		printf("%s component->rfet1     = true, end=%d\n", str, component->rfet1.end!=NULL);
	if(component->rfet2.ptr)
		printf("%s component->rfet2     = true, end=%d\n", str, component->rfet2.end!=NULL);

	if(*component->para1)
		printf("%s component->para1     = true\n", str);
	if(*component->para2){
		printf("%s component->para2     = true\n", str);}

		printf("%s component->access    = %s\n", str, access2str(c_access(component)));
		printf("%s component->state     = %s\n", str, state2str(component->state));
		printf("%s component->parent    = %p\n", str, (void*)component->parent);

	if(component->oper1)
		printf("%s component->oper1     = %p\n", str, (void*)component->oper1);
	if(component->oper2)
		printf("%s component->oper2     = %p\n", str, (void*)component->oper2);

	if(component->replace)
		printf("%s component->replace   = %d\n", str, component->replace);

	if(component->depOnMe.size)
		printf("%s component->depOnMe   = %ld\n", str, component->depOnMe.size);

	if(component->depend1.size)
		printf("%s component->depend1   = %ld\n", str, component->depend1.size);

	if(component->depend2.size)
		printf("%s component->depend2   = %ld\n", str, component->depend2.size);

	if(component->replacement.size)
		printf("%s component->replacement = %ld\n", str, component->replacement.size);

	if(component->inherits.size)
		printf("%s component->inherits  = %ld\n", str, component->inherits.size);

	if(component->inners.size)
		printf("%s component->inners    = %ld\n", str, component->inners.size);

	printf("\n");

	if(indent>=-1)
	{
		if(indent<0) indent--; else indent++;
		void* c = avl_min((AVLT*)(size_t)&component->inners);
		for( ; c != NULL; c = avl_next(c))
			component_print(text, indent, (Component*)c);
	}
}

/***********************************************************************************************************/


bool CheckStr3 (const_Str3 str)
{
	bool success = false;
	do{
		if(strEnd3(str)) { success=true; break; }
		if(str.ptr==NULL && str.end!=NULL) {assert(false); break;}

		lchar* lchr = str.ptr->prev;
		if(lchr && lchr->next != str.ptr) {assert(false); break;}

		while(true)
		{
			lchr = str.ptr->next;
			if(lchr && lchr->prev != str.ptr) {assert(false && "Str3 pointers are invalid"); break;}
			if(strEnd3(str)) { success = true; break; }
			else if(lchr==NULL) {assert(false && "mallocated string does not end with '\0'."); break;}
			str.ptr = lchr;
		}
	}while(0);
	return success;
}


static bool CheckAVLT (const AVLT tree)
{
	bool success = avl_valid(&tree);
	assert(success && "CheckAVLT() has failed failed");
	return success;
}


static bool CheckPara (const_value vst)
{
	bool success = false;
	do{
		if(!vst) { success=true; break; }
		// ... ...
		success = true;
	}while(0);
	return success;
}


static bool CheckOper (const_value opr)
{
	bool success = false;
	do{
		if(!opr) { success=true; break; }
		// ... ...
		success = true;
	}while(0);
	return success;
}


bool CheckComponent (Component* component, bool finalised)
{
 bool success = false;
 do{
	if(!component) { success=true; break; }
	if(finalised){
		if(component->name2.ptr) {assert(false); break;}
		if(component->text2.ptr) {assert(false); break;}
		if(component->rfet2.ptr) {assert(false); break;}
		//if(component->para2[0]) {assert(false); break;} // do not include this due to a constant-type
		if(component->oper2) {assert(false); break;}
		if(component->depend2.size) {assert(false); break;}
	}
	if(!CheckStr3(component->name1)) break;
	if(!CheckStr3(component->name2)) break;
	if(!CheckStr3(component->text1)) break;
	if(!CheckStr3(component->text2)) break;
	if(!CheckStr3(component->rfet1)) break;
	if(!CheckStr3(component->rfet2)) break;
	if(!CheckPara(component->para1)) break;
	if(!CheckPara(component->para2)) break;
	if(!CheckOper(component->oper1)) break;
	if(!CheckOper(component->oper2)) break;
	if(!CheckAVLT(component->depend1)) break;
	if(!CheckAVLT(component->depend2)) break;
	if(!CheckAVLT(component->depOnMe)) break;
	if(!CheckAVLT(component->inherits)) break;
	if(!CheckAVLT(component->replacement)) break;
	if(!CheckAVLT(component->inners)) break;

	void* node;

	if(component->rfet1.ptr
	&& !list_find(container_list(), NULL, pointer_compare, &component)) {assert(false); break;}

	node = avl_min(&component->inherits);
	for( ; node != NULL; node = avl_next(node))
	{
		Component* c = *(Component**)node;
		if(c->type1 != component) {assert(c->type1 == component); break;}
	}

	node = avl_min(&component->depOnMe);
	for( ; node != NULL; node = avl_next(node))
	{
		Component* c = *(Component**)node;
		if(!avl_do(AVL_FIND, &c->depend1, &component, 0, 0, pointer_compare)
		/*&& !avl_do(AVL_FIND, &c->depend2, &component, 0, 0, pointer_compare)*/)
			{assert(false && "error on AVL_FIND  c->dependX  &component"); break;}
	}

	node = avl_min(&component->depend1);
	for( ; node != NULL; node = avl_next(node))
	{
		Component* c = *(Component**)node;
		if(!avl_do(AVL_FIND, &c->depOnMe, &component, 0, 0, pointer_compare))
			{assert(false && "error on AVL_FIND  c->depOnMe  &component"); break;}
	}

	if(component->type1
	&& !avl_do(AVL_FIND, &component->type1->inherits, &component, 0,0,0)) {assert(false); break;}

	if(component->parent
	&& !avl_do(AVL_FIND, &component->parent->inners, component, 0,0,0)) {assert(false); break;}

	node = avl_min(&component->inners);
	for( ; node != NULL; node = avl_next(node))
	{
		Component* c = (Component*)node;
		if(c->parent != component)
			{ assert(c->parent == component); break; }
		if(!CheckComponent(c, finalised)) {assert(false); break;} // recursive call
	}

	success = true;
 }while(0);
 return success;
}

