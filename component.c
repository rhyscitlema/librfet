/*
	component.c
*/

#include <_stdio.h>
#include "outsider.h"
#include "component.h"
#include "expression.h"
#include "operations.h"

static List _container_list = {0};
List* container_list() { return &_container_list; }

static Container _rootContainer;
static Container *rootContainer = NULL; // see component_new()

int pointer_compare (const ITEM* item1, const ITEM* item2, const void* arg)
{
	const void* a = *(const void**)item1;
	const void* b = *(const void**)item2;
	return a<b ? -1 : a>b ? +1 : 0;
}

static Container *do_container_find (value stack, Container* current, const_Str3 pathname, bool skipFirst, bool fullAccess, bool type1only);

#define MAX_LINEAGE 15

/**************************************************************************************************************/


/* read: I 'component', am depending on 'depend' */
void depend_add (Component* component, Component *depend)
{
	assert(depend!=NULL);
	if(component && component!=depend)
		tree_do(
			TREE_ADD,
			&component->depend2,
			&depend,
			sizeof(Component*),
			NULL,
			pointer_compare);
}


/* read: I 'depend', am depended upon by 'component' */
static void depend_notify (Tree* depend, const Component *component)
{
	void* node = tree_first(depend);
	for( ; node != NULL; node = tree_next(node))
	{
		Component* c = *(Component**)node;
		tree_do(
			TREE_ADD,
			&c->depOnMe,
			&component,
			sizeof(component),
			NULL,
			pointer_compare);
	}
}


/* read: I 'depend', am not depended upon by 'component' */
void depend_denotify (Tree* depend, const Component *component)
{
	void* node = tree_first(depend);
	for( ; node != NULL; node = tree_next(node))
	{
		Component* c = *(Component**)node;
		tree_do(
			TREE_DEL,
			&c->depOnMe,
			&component,
			sizeof(component),
			NULL,
			pointer_compare);
	}
}

/**************************************************************************************************************/


static Str3 setEnd (Str3 str)
{
	assert(str.ptr != NULL);
	if(str.ptr && !str.end)
	{
		str.end = str.ptr;
		while(str.end->chr.c) // get to char '\0'
			str.end = str.end->next;
	}
	return str;
}

// NOTE: if doing component_evaluate() before component_finalise(),
// then assume component->replace to != 0 inside operations_evaluate().
static void update_replace_count (Component* component, int offset)
{
	Component *c = do_container_find(NULL, component, c_name(component), true, false, offset<0);
	assert(c!=NULL); if(c) { c->replace += offset; assert(c->replace >= 0); }
}

bool CheckRoot (bool success, int type)
{
	if(type>=3){
		component_print(" |  ", 0, rootContainer);
		if(type<2) puts2(L"--------------------------------------------------------\n");
		else puts2(L"============================================================================\n");
	}
	return CheckComponent(rootContainer, success);
}


static void component_finalise (Component *component, bool success)
{
	assert(CheckRoot(false, 0));
	if(component==NULL) return;
	assert(component->state!=ISPARSE);
	assert(component->parent!=NULL);
	if(component->parent==NULL) return;
	component->instance = 0;
	if(success)
	{
		if(component->state==NOFOUND)
		{
			// Note: the 'if' below is not placed inside component_destroy() because
			// all other calls to it are guaranteed to remove the inheritance hierarchy.
			// Also for this same reason a crash may occured due to accessing freed component.
			if(component->access1 == ACCESS_REPLACE)
				update_replace_count(component, -1);
			// line above must come first
			component_destroy(component);
		}
		else
		{
			if(strEnd3(component->name2))   // if was not parsed,
			{
				// then a different one that got parsed depends on this component
				assert(component->name2.ptr==NULL && component->name2.end==NULL);
				assert(component->text2.ptr==NULL && component->text2.end==NULL);
				assert(component->para2[0] == 0);
				assert(component->type2 == NULL);
				assert(component->rfet2.ptr == NULL);
			}
			else
			{
				if(component->state==CREATED && c_iscont(component))
					list_head_push(container_list(), list_new(&component, sizeof(component)));

				// if conversion from container to non-container
				if(component->state==DOPARSE
				&& component->rfet1.ptr!=NULL   // if was a container
				&& component->rfet2.ptr==NULL)  // if not a container
				{
					// then remove container from list of containers
					void* node = list_find(container_list(), NULL, pointer_compare, &component);
					assert(node!=NULL); list_delete(container_list(), node);

					// also remove inners of this non-container
					Component c = {{0}};
					c.inners = component->inners;
					component_destroy(&c);
				}

				if(component->access1 != ACCESS_REPLACE
				&& component->access2 == ACCESS_REPLACE) update_replace_count(component, +1);

				if(component->access1 == ACCESS_REPLACE
				&& component->access2 != ACCESS_REPLACE) update_replace_count(component, -1);

				assert(component->name2.ptr != component->name1.ptr); str3_free(component->name1);
				assert(component->text2.ptr != component->text1.ptr);
				assert(!component->text2.ptr == !component->text2.end);
				component->name1 = component->name2; component->name2 = C37(NULL);
				component->text1 = component->text2; component->text2 = C37(NULL);
				memcpy(component->para1, component->para2, sizeof(component->para2));
				*component->para2 = 0;

				Str3 r1 = component->rfet1;
				Str3 r2 = component->rfet2;     // replace rfet inside that
				if(r1.end && r2.ptr && !r2.end) // of the parent container
				{
					r2 = setEnd(r2);
					assert(!strEnd3(r2));
					assert(r2.ptr->prev==NULL);

					lchar *chr;
					chr = r1.ptr->prev;
					r2.ptr->prev = chr;
					chr->next = r2.ptr;

					chr = r1.end->next;
					r2.end->next = chr;
					chr->prev = r2.end;

					assert(r2.end->chr.c =='\0');
					assert(r1.end->chr.c == '}');
					r2.end->chr.c = r1.end->chr.c;
					r1.end->chr.c = '\0';

					r1.ptr->prev = NULL;
					r1.end->next = NULL;
					r1.end = NULL; // prepare to free r1

					assert(CheckStr3(r1));
					assert(CheckStr3(r2));
				}
				r1 = str3_free(r1);
				component->rfet1 = r2;
				component->rfet2 = C37(NULL);

				if(component->type1 != component->type2)
				{
					if(component->type1 && !tree_do(TREE_DEL, &component->type1->inherits, &component, 0,0,0)) {assert(false);}
					if(component->type2 &&  tree_do(TREE_ADD, &component->type2->inherits, &component, 0,0,0)) {assert(false);}
				}
				component->type1 = component->type2;
				component->type2 = NULL;

				component->access1 = component->access2;
			}
			tree_free(&component->replacement);

			depend_denotify(&component->depend1, component);
			depend_notify  (&component->depend2, component);
			tree_free(&component->depend1);
			component->depend1 = component->depend2;
			tree_clear(&component->depend2);

			assert(component->oper2!=NULL);
			value_free(component->oper1);
			component->oper1 = component->oper2;
			component->oper2 = NULL;

			component->state = ISPARSE;
		}
	}
	else
	{
		if(component->state==CREATED)
			component_destroy(component);
		else
		{
			assert(component->name2.ptr != component->name1.ptr);
			assert(component->text2.ptr != component->text1.ptr);
			assert(!component->text2.ptr == !component->text2.end);

			str3_free(component->name2);
			str3_free(component->rfet2);
			value_free(component->oper2);

			component->name2 = C37(NULL);
			component->text2 = C37(NULL);
			component->rfet2 = C37(NULL);
			component->oper2 = NULL;
			*component->para2 = 0;

			tree_free(&component->replacement);
			tree_free(&component->depend2);
			component->type2 = NULL;

			component->state = ISPARSE;
		}
	}
	assert(CheckRoot(false, 1));
}


typedef struct { Component* component; int children; } CompDep;

static Tree dependence = { 0, 0, sizeof(CompDep), NULL, pointer_compare };

static void dependence_notify (Component *component)
{
	if(!component) return;
	CompDep cp = { component, 0 };

	if(!tree_do(TREE_ADD, &dependence, &cp, 0,0,0))
		memory_alloc("Dependence"); // on a new addition

	if(component->state==CREATED) return;
	if(component->state!=NOFOUND
	&& component->state!=ISFOUND
	&& component->state!=DOFOUND)
	   component->state =DOPARSE;

	void* node = tree_first(&component->depOnMe);
	for( ; node != NULL; node = tree_next(node))
		dependence_notify(*(Component**)node);
}


void dependence_finalise (bool success)
{
	void* ptr = tree_first(&dependence);
	for( ; ptr!=NULL; ptr = tree_next(ptr))
	{
		CompDep* cp = (CompDep*)ptr;
		Component* comp = cp->component;
		cp = (CompDep*)tree_do(TREE_FIND, &dependence, &comp->parent, 0,0,0);
		if(cp) cp->children++;
	}

	ptr = tree_first(&dependence);
	for( ; ptr!=NULL; ptr = tree_next(ptr))
	{
		CompDep* cp = (CompDep*)ptr;
		while(cp && cp->children==0)
		{
			cp->children--; // prevent any future re-processing
			Component* comp = cp->component;
			cp = (CompDep*)tree_do(TREE_FIND, &dependence, &comp->parent, 0,0,0);

			if(cp) cp->children--;
			component_finalise(comp, success);
			memory_freed("Dependence");
		}
	}
	tree_free(&dependence);
	assert(CheckRoot(true, 2));
}


bool dependence_parse (value stack)
{
	void* ptr = tree_first(&dependence);
	for( ; ptr!=NULL; ptr = tree_next(ptr))
		if(!component_parse(stack, *(Component**)ptr))
			return false;
	/*ptr = tree_first(&dependence);
	for( ; ptr!=NULL; ptr = tree_next(ptr))
		if(!component_evaluate(...))
			return false;*/
	return true;
}

/**************************************************************************************************************/

typedef struct _Inner {
	Str3 name, text, para;
	enum COMP_ACCESS access;
} Inner;


static bool component_extract (value stack, const_Str3 input, List* inners)
{
	const Expression *nextItem = NULL;
	bool is_main_comp = true;
	assert(inners!=NULL);

	if(strEmpty3(input,0)) { setError(stack, L"Empty input."); return false; }
	setBool(stack, true); // used by return statement at the end

	while(!strEnd3(input)) // while getting components
	{
		/* Step0: initialise */
		const_Str3 str = {0};
		Inner inner = {{0}};
		inner.access = is_main_comp ? ACCESS_PRIVATE : ACCESS_PROTECTED;

		if(!is_main_comp)
		{
			/* Step1: Obtain component access control */
			nextItem = get_next_item (stack, &input, &str, 0,0);
			if(nextItem==NULL || strEnd3(input)) break;

			int access = -1;
			if(0==strcmp31(str,"private"  )) access = ACCESS_PRIVATE;
			if(0==strcmp31(str,"enclosed" )) access = ACCESS_ENCLOSED;
			if(0==strcmp31(str,"protected")) access = ACCESS_PROTECTED;
			if(0==strcmp31(str,"public"   )) access = ACCESS_PUBLIC;
			if(0==strcmp31(str,"replace"  )) access = ACCESS_REPLACE;

			/* Step2: Obtain the component */
			if(access != -1)
			{
				inner.access = access;
				nextItem = get_next_item (stack, &input, &str, 0,0);
				if(nextItem==NULL || strEnd3(input)) break;
			}

			/* Step2.0: Obtain component as an inner-container */
			if(0==strcmp31(str,"\\rfet"))
			{
				if(sChar(input) != '{')
				{ setErrorE(stack, L"Error at (%|2s,%s) in %s:\r\nExpected '{' directly after \\rfet.", input); break; }

				input = sNext(input);       // skip '{'
				str.ptr = input.ptr;        // set start at after '{'
				int brackets=1;
				while(brackets)
				{
					if(strEnd3(input)) { setErrorE(stack, L"Error at (%|2s,%s) in %s:\r\nExpected a closing '}'.", input); break; }
					wchar c = sChar(input);
					if(c=='{') brackets++;  // TODO: also skip "...{..." and '{'
					if(c=='}') brackets--;
					input = strNext3(input);
				}
				if(brackets) break;         // if error
				str.end = sPrev(input).ptr; // set end at before '}'

				inner.text = str;
				list_tail_push(inners, list_new(&inner, sizeof(inner)));
				continue;
			}

			if(sChar(str)=='\\')
			{ setErrorE(stack, L"Error on '%s' at (%s,%s) in %s:\r\nExpected \\rfet{ only.", str); break; }

			if(nextItem->ID != SET_PARAMTER)
			{ setErrorE(stack, L"Error on '%s' at (%s,%s) in %s:\r\nExpected a component name instead.", str); break; }

			else inner.name = str;

			nextItem = get_next_item (stack, &input, &str, 0,0);
			if(nextItem==NULL || strEnd3(input)) break;
		}
		else if(sChar(input)=='\\' && sChar(sNext(input))=='(') // if main component as a function
		{
			is_main_comp = false;
			input = sNext(input); // skip the '\\'
			nextItem = get_next_item (stack, &input, &str, 0,0); // get the '('
			assert(nextItem!=NULL);
		}

		if(!is_main_comp)
		{
			/* Step3: Obtain component parameter */
			if(sChar(str) == '(')
			{
				// if '(' then obtain parameter structure
				int brackets=1;
				while(brackets)
				{
					if(strEnd3(input)) { setErrorE(stack, L"Error at (%|2s,%s) in %s:\r\nExpected a closing ')'.", input); break; }
					wchar c = sChar(input);
					if(c=='(') brackets++;
					if(c==')') brackets--;
					input = strNext3(input);
				}
				if(brackets) break;     // if error
				str.end = input.ptr;    // set end at after ')'
				inner.para = str;

				nextItem = get_next_item (stack, &input, &str, 0,0); // get the '=' assignment operator
				if(!nextItem) { str = input; assert(strEnd3(input)); }
			}

			/* Step 4: final check for a '=' assignment operator */
			if(nextItem==NULL || sChar(str) != '=')
			{ setErrorE(stack, L"Error on '%s' at (%s,%s) in %s:\r\nExpected assignment '=' instead.", str); break; }
		}
		else is_main_comp = false;

		/* Step 5: collect the RFET */
		str.ptr = input.ptr;
		do{ const_Str3 s={0}; // not used
			nextItem = get_next_item (stack, &input, &s, 0,0);
		} while(nextItem && nextItem->ID != EndOfStatement);
		str.end = input.ptr;
		if(nextItem) str.end = str.end->prev; // unskip the ';'

		/* Step 7: iterative tree-traversal to record sub-components of a component's result-structure */
		// not implemented

		inner.text = str;
		list_tail_push(inners, list_new(&inner, sizeof(inner)));
	}
	return VTYPE(*stack)!=VALUE_MESSAGE; // return true if no error
}


static int tree_component_compare (const ITEM* item1, const ITEM* item2, const void* arg)
{ return strcmp33( c_name((const Component*)item1), c_name((const Component*)item2) ); }

// note: after call, set name to NULL
static Component* component_new (Container* parent, Str3 name)
{
	Component *component;
	if(parent==NULL){
		assert(rootContainer==NULL);
		component = rootContainer = &_rootContainer;
	}else component = (Component*)tree_new(NULL, sizeof(Component));
	memory_alloc("Component");
	component->parent = parent;
	component->state = CREATED;
	component->name2 = name;
	component->inherits.itemsize = sizeof(Component*);
	component->inherits.compare = pointer_compare;
	component->inners.itemsize = sizeof(Component);
	component->inners.compare = tree_component_compare;
	if(parent) tree_do(TREE_PUT, &parent->inners, component, 0,0,0);
	return component;
}


static bool check_access(value stack, int from, int to, const_Str3 name)
{
	if(from <= to) return true;
	const_Str2 argv[3];
	Str2 msg = (Str2)(stack+10000); // +10000 space reserved for result
	argv[1] = msg; msg = 1+strcpy21(msg, access2str(from));
	argv[2] = msg; msg = 1+strcpy21(msg, access2str(to)); // 1+ so to skip '\0'
	argv[0] = L"Error on '%s' at (%s,%s) in %s:\r\nCannot downgrade access from %s to %s.";
	setMessageE(stack, 0, 3, argv, name);
	return false;
}


static bool component_insert (value stack, Container* container, List* inners)
{
	Inner* inner = (Inner*)list_head(inners);

	Component* component;
	Str3 name, text, para;
	enum COMP_ACCESS access;

	bool error = container==NULL;
	if(container)
	{
		component = (Component*)tree_first(&container->inners);
		for( ; component != NULL; component = (Component*)tree_next(component))
		{
			     if(component->state==ISPARSE) component->state = ISFOUND;
			else if(component->state==DOPARSE) component->state = DOFOUND;
			else{
				component_print("component_insert ", -1, component);
				assert(component->state==ISPARSE || component->state==DOPARSE);
			}
		}
	}
	if(!error)
	while(inner)
	{
		name  = inner->name;
		text  = inner->text;
		para  = inner->para;
		access= inner->access;
		inner = (Inner*)list_next(inner);

		if(text.end->chr.c=='}' // if an inner container
		&& sChar(sPrev(text))=='{')
		{
			Container *cont = container_parse(stack, container, C37(NULL), text);
			if(!cont) { error=true; break; }
			else cont->access2 = access;
			continue;
		}

		uint32_t param[1000];
		if(strEnd3(para))       // if component is a variable
			param[0] = 0;       // then mark as variable
		else                    // else component is a function
		{
			param[0] = 1;       // assume a function with 0 paras
			//para.ptr = para.ptr->next;  // remove '('
			//para.end = para.end->prev;  // remove ')'
			//if(!strEmpty3(para,0))
			{
				if(VERROR(parseExpression(stack, para, NULL))) { error=true; break; }
				assert(stack == vGet(stack));
				value v = 1+param;

				memset(v, 0, sizeof(OperEval));
				if(VERROR(operations_evaluate(v, stack)))
				{ assert(false); error=true; break; }

				assert(v==vGet(v)); // TODO: TODO: call value_resolve()

				long size = 1+vSize(v); // 1+ since param[0] is a separate data
				if(size >= SIZEOF(component->para2))
				{
					setErrorE(stack, L"Error at (%|2s,%s) in %s:\r\nFunction parameter is too long.", para);
					error=true; break;
				}
				const_value n=v;
				while(n < param+size) // count the number of paras
				{
					if(VTYPE(*n)==VALUE_VECTOR)
						n += 2; // skip vector header
					else {
						param[0]++; // count para
						assert(isStr2(n));
						n = vNext(n);
					}
				}
			}
		}

		if(name.ptr==NULL) component = container; // if is_main_comp
		else
		{
			// set to private irrespective of user-defined access type
			if(0==strcmp31(name,"name")) access = ACCESS_PRIVATE;

			/* Step 6: record the extracted component */
			component = component_find (stack, container, name, 0);
			int i = component==NULL ? 0                 // if not already defined
			      : component->parent!=container ? 1    // if inheriting from a sibling container
			      : 2 ;

			if(i==1 && !check_access(stack, c_access(component), access, name))
			{ error=true; break; }

			else if(i!=2)
			{
				component = component_new(container, name);
				name=C37(NULL);
			}
			else if(!strEnd3(component->name2)) // if already exists
			{
				setErrorE(stack, TWST(Component_Already_Defined), name);
				error=true; break;
			}
			//else if(component->state==ISFOUND
			//     && 0==strcmp33(c_text(component), text)        // if exact same expression
			//     && 0==value_compare(c_para(component), param))  // and parameter, then no parse
			//     component->state = NOPARSE; // TODO: but expression.name needs to be updated

			else component->state = DOPARSE;

			if(name.ptr){ // if not a new component
				assert(component->name2.ptr==NULL);
				component->name2 = name;
			}
			component->access2 = access;
		}
		//if(param && sChar(text) == '{' && sChar(sPrev(text)) != '=')
		//    component->instance = 1; // mark component as a procedure

		memcpy(component->para2, param, sizeof(component->para2));
		assert(component->text2.ptr==NULL);
		component->text2 = text;

		if(component->state!=NOPARSE)
			dependence_notify(component);
	}
	if(container)
	{
		component = (Component*)tree_first(&container->inners);
		for( ; component != NULL; component = (Component*)tree_next(component))
		{
			     if(component->state==ISFOUND) component->state = error ? ISPARSE : NOFOUND;
			else if(component->state==DOFOUND) component->state = error ? DOPARSE : NOFOUND;
			else continue; // avoid line below
			if(!error) dependence_notify(component);
		}
	}
	list_free(inners);
	return !error;
}


bool isSpecial3 (const_Str3 name)
{
	int dots = 0;
	const_Str3 s;
	for(s=name; !strEnd3(s); s = sNext(s))
	{
		wchar c = sChar(s);
		if(c=='|') { dots=0; break; }
		else if(c=='.') dots++;
		else dots = 3;
	}
	return dots<3;
}
#define Component_not_be_Special L"Error on '%s' at (%s,%s) in %s:\r\nMust not contain '|' and not be \"\", \".\" nor \"..\"."


static bool get_string_expr (value stack, const_Str3 strExpr, const_Str3* name)
{
	const_Str2 argv[2];
	const_Str3 s = strExpr;
	const Expression* expression = get_next_item (stack, &s, name, 0,0);

	// note: 's' now points to after the collected 'name'
	while(!strEnd3(s) && isSpace(sChar(s))) s = sNext(s);
	if(!strEnd3(s)) expression = NULL;

	if(!expression || !name->ptr || sChar(*name) != '"')
	{
		argv[0] = L"Error on '%s' at (%s,%s) in %s:\r\nGiven name must be a direct string.";
		setMessageE(stack, 0, 1, argv, strExpr); return false;
	}
	name->ptr = name->ptr->next; // skip starting "
	name->end = name->end->prev; // skip ending "
	assert(name->end->chr.c == '"');

	long size = strlen3(*name);
	if(size > MAX_NAME_LEN)
	{
		argv[0] = L"Error at (%|2s,%s) in %s:\r\nString of length %s is too long.";
		argv[1] = TIS2(0,size);
		setMessageE(stack, 0, 2, argv, *name); return false;
	}
	if(isSpecial3(*name))
	{
		argv[0] = Component_not_be_Special;
		setMessageE(stack, 0, 1, argv, *name); return false;
	}
	return true;
}


Container* container_parse (value stack, Container* parent, Str3 name, Str3 text)
{
	if(!rootContainer) rootContainer = component_new(NULL, C37(NULL));
	if(!parent) parent = rootContainer;
	Container* container;
	List _inners={0}, *inners = &_inners;

	bool skip = false;
	bool found = false;
	bool success = false;
	const_Str3 type = {0};
	const_Str2 argv[2];

	do{
		int i=0; container = parent;
		for( ; container != rootContainer; container = container->parent) i++;
		if(i>=MAX_LINEAGE) { setErrorE(stack, L"Error at (%|2s,%s) in %s:\r\nInner container is too deep down the lineage.", text); break; }
		container=NULL;

		if(!strEnd3(name)){
			const_Str3 lstr = name; if(sChar(lstr)=='|') lstr = sNext(lstr);
			if(isSpecial3(lstr)) { setErrorE(stack, Component_not_be_Special, name); break; }
		}
		if(!strEnd3(text))
		{
			if(!component_extract (stack, text, inners)) break;
			const_Str3 lstr = {0};

			Inner* inner = (Inner*)list_head(inners);
			for( ; inner; inner = (Inner*)list_next(inner))
			{
				if(0==strcmp31(inner->name, "name")) lstr = inner->text;
				if(0==strcmp31(inner->name, "type")) type = inner->text;
			}
			if(!strEnd3(lstr)) // if component 'name' was extracted
			{
				name = str3_free(name);
				if(!get_string_expr(stack, lstr, &name)) break;
				else found=true; // mark that the container name is found
			}
		}
		else assert(!strEnd3(name)); // name and text cannot be both empty

		if(!strEnd3(name))
		{
			Container cont; memset(&cont,0,sizeof(cont));
			cont.parent = parent;
			container = container_find(NULL, &cont, name, 0);
			if(container && container->parent != parent) { skip=true; container=NULL; }
			// TODO: provide comment of when above if() evaluates to true
		}
		if(strEnd3(text)) // if no text then name = "|filename"
		{
			if(container) { success=true; break; } // if file was loaded before

			if(sChar(name)!='|') break;  // File name must start with '|'
			name = sNext(name); // skip the '|'

			if(VERROR(FileOpen2(C23(name), stack))) break;

			text = astrcpy32(text, getStr2(stack), C23(name));

			if(name.end==NULL) // if name is a mallocated Str3
			{
				Str3 s = sPrev(name); // prepare to free '|'
				name.ptr->prev = NULL;
				assert(s.ptr->prev==NULL);
				s.ptr->next = NULL;
				s = str3_free(s); // do free the '|'
			}
			else // else make name mallocated since container is of top-level
			{
				Str3 s = name;
				name = str3_alloc(C37(NULL), strlen3(s));
				strcpy33(name, s);
			}

			if(!component_extract (stack, text, inners)) break;
		}

		if(!found && parent!=rootContainer) { setErrorE(stack, L"Error at (%|2s,%s) in %s:\r\nNon-top-level container must have a name.", text); break; }
		found = !strEnd3(type); // mark that the container type is found

		if(!container)
		{
			if(strEnd3(name)) assert(parent != rootContainer);      // assert fails if container is top-level and name==NULL yet text has no 'name' component
			else if(checkfail(stack, parent, name, found, true)) break;   // ...this will typically happen if this function is called with a name==NULL parameter.
			container = component_new(parent, name); name=C37(NULL);
		}
		else if(checkfail(stack, container, name, found, false)) break;
		else{
			container->state = DOPARSE;
			container->name2 = name; name=C37(NULL);
		}

		if(found) // if container type was extracted
		{
			if(!get_string_expr(stack, type, &name)) break;
			type = name;

			bool same = 0==strcmp33(type, container->name2);
			if(!skip) skip = same;
			else if(!same)
			{ setErrorE(stack, L"Error on '%s' at (%s,%s) in %s:\r\nThe type must be same as the name so to override.", type); break; }

			Container *cont_type = do_container_find(stack, container, type, skip, 0, 0);
			assert(cont_type != container);
			if(!cont_type) // concatenate onto VALUE_MESSAGE
			{
				argv[0] = L"%s\r\nSo cannot access the sibling container.";
				argv[1] = getMessage(stack); // get error from calling do_container_find()
				vPrevCopy(stack, setMessage(vnext(stack), 0, 2, argv));
				break;
			}

			if(*c_para(cont_type))
			{ setErrorE(stack, L"Error on '%s' at (%s,%s) in %s:\r\nCannot inherit a sibling that takes parameters.", type); break; }

			else container->type2 = cont_type;
		}
		else container->type2 = NULL; // else this container does not inherit

		success = true;
	}while(0);

	if(container){
		container->rfet2 = text; text=C37(NULL);
		dependence_notify(container);
	}
	// note: this call must always be done so to free innerFunctions and innerContainers
	if(!component_insert(stack, success?container:NULL, inners))
		container=NULL;

	if(container && parent==rootContainer) container->access2 = ACCESS_PUBLIC; // if a top-level container

	//component_print(" {  ", 1, container);
	//puts2(L"------------------------------------------------------------\n");

	name = str3_free(name);
	text = str3_free(text);
	return container;
}

/**************************************************************************************************************/


static int tree_container_find (const ITEM* item1, const ITEM* item2, const void* arg)
{ return strcmp33( *(const Str3*)item1, c_name((const Component*)item2) ); }

static Container *do_container_find (value stack, Container* current, const_Str3 pathname, bool skipFirst, bool fullAccess, bool type1only)
{
	if(!rootContainer) rootContainer = component_new(NULL, C37(NULL));
	if(strEnd3(pathname)) return rootContainer;
	if(!current) current = rootContainer; // if this then pathname must start with '|'

	const_Str2 argv[5];
	const_Str3 name = {0};
	unsigned access = ACCESS_PRIVATE;
	bool firsttime = true;
	for( ; ; firsttime=false)
	{
		if(strEnd3(pathname)) break; // quit main loop

		name.ptr = pathname.ptr;
		while(!strEnd3(pathname) && sChar(pathname) != '|')
			pathname = sNext(pathname);
		name.end = pathname.ptr;

		if(!strEnd3(pathname))
			pathname = sNext(pathname); // skip the '|'

		if(strEnd3(name)) {
			if(firsttime) {
				current = rootContainer;
				access = ACCESS_PUBLIC;
			}
			continue;
		}
		else if(0==strcmp31(name, "." )) { continue; }
		else if(0==strcmp31(name, "..")) {
			if(!(current = current->parent))
				current = rootContainer;
			continue;
		}
		else if(firsttime) current = current->parent; // default goes to parent

		if(0==strcmp31(name,"main")){
			if(current==rootContainer){
				current=NULL;
				setError(stack, L"Error: root has no main...!");
			}
			break; // quit main loop
		}

		Container *from = current;
		Container *c=NULL;
		while(true)
		{
			if(skipFirst) skipFirst=false;
			else{
				c = (Container*) tree_do (TREE_FIND, &current->inners, &name, 0, NULL, tree_container_find);
				if(c!=NULL && c->state != NOFOUND)
				{
					enum COMP_ACCESS acc = c_access(c);
					if(!fullAccess && acc < access) // TODO: do_container_find(), upon replace-access-type, should maybe search for the replaced component and use its acces-type instead?
					{
						if(!stack) { c=NULL; break; }
						argv[0] = L"Error on '%s' at (%s,%s) in %s:\r\n"
								  L"Component has %s access inside %s.\r\n"
								  L"Expected at least %s access.";
						Str2 msg = (Str2)(stack+10000); // +10000 space reserved for result
						argv[1] = msg; msg = 1+strcpy21(msg, access2str(acc)); // 1+ so to skip '\0'
						argv[3] = msg; msg = 1+strcpy21(msg, access2str(access));
						argv[2] = C23(c_name(current));
						setMessageE(stack, 0, 4, argv, name);
						c=NULL; // mark error
					}
					break;
				}
				else assert(true || !(c!=NULL && c->state != NOFOUND));
			}

			c = current;
			current = type1only ? c->type1 : c_type(current);
			if(current==NULL)
			{
				argv[1] = (from==rootContainer) ? L"|" : C23(c_name(from));
				argv[0] = TWST(Cannot_Find_Component);
				setMessageE(stack, 0, 2, argv, name);
				c=NULL; // mark error
				break;
			}

			const Container *p1 = current->parent;
			const Container *p2 = c->parent;

			if(p1 == p2) { // if have same parent
				if(access <= ACCESS_PRIVATE)
					access = ACCESS_ENCLOSED;
			}
			else if(p1->parent == p2->parent) { // if have same grandpa
				if(access <= ACCESS_ENCLOSED)
					access = ACCESS_PROTECTED;
			}
			else access = ACCESS_PUBLIC;
		}
		current = c;
		if(c==NULL) break; // quit main loop
		if(access < ACCESS_PUBLIC)
			access++; // prepare for next loop
	}
	if(current) setBool(stack, true);
	return current;
}


Container *container_find (value stack, Container* current, const_Str3 pathname, bool fullAccess)
{ return do_container_find (stack, current, pathname, false, fullAccess, false); }

Component *component_find (value stack, Container* current, const_Str3 name, bool skipFirst)
{
	if(current==NULL) return NULL;
	if(strEnd3(name)) return NULL;
	lchar lstr[2];
	lstr[0].chr = mChar(name);
	lstr[1].chr = lstr[0].chr;
	lstr[0].chr.c = '.';
	lstr[1].chr.c = '|';
	lstr[0].next = lstr+1;
	lstr[1].next = name.ptr;
	const_Str3 pathname = {lstr, name.end};
	return do_container_find (stack, current, pathname, skipFirst, false, false);
}


/* build and return the full path of given container */
Str2 container_path_name (value stack, const Container* container)
{
	const Container* cont[MAX_LINEAGE+1];
	if(!container) container = rootContainer;

	int i;
	for(i=0; i<=MAX_LINEAGE && container!=rootContainer; container = container->parent)
		cont[i++] = container;
	assert(container==rootContainer);

	Str2 out = (Str2)(stack+2);
	if(i==0) *out = 0;
	while(i--)
	{
		*out++ = '|';
		out = strcpy23(out, c_name(cont[i]));
	}
	onSetStr2(stack, out);

	stack[1] |= 1; // see bool fullAccess in operations.c
	return (Str2)(stack+2);
}

/**************************************************************************************************************/


/* parse the expression-string of a component into an operations-array */
Component* component_parse (value stack, Component *component)
{
	if(component==NULL) return NULL;

	if(component->state == ISPARSE
	|| component->state == NOPARSE
	|| component->state == NOFOUND
	|| component->oper2 != NULL) return component;

	Component* comp = do_container_find(stack, component, c_name(component), true, false, false);
	if(comp){
		depend_add(component, comp);
		if(!check_access(stack, c_access(comp), c_access(component), c_name(component))) return NULL;
	}
	else if(c_access(component)==ACCESS_REPLACE) return NULL;

	//assert(component->depend2.size==0); // must be 0 before parsing (but there is access_replace)
	component->oper2 = (value)1; // prevent recursive parsing on recursive call

	if(VERROR(parseExpression(stack, c_text(component), component)))
	{ component->oper2 = NULL; return NULL; }
	assert(stack == vGet(stack));

	long size = vSize(stack);
	component->oper2 = value_alloc(NULL, size);
	memcpy(component->oper2, stack, size*sizeof(*stack));

	return component;
}


// see expression_to_operation() inside expression.c
static inline value setOpers (value v, int oper, int size)
{ *v = (VALUE_OPERAT<<28) | (oper<<16) | size; return v+1+size; }

value component_evaluate (
	value stack,
	Container *caller,
	Component *component,
	const_value argument)
{
	assert(stack!=NULL);
	assert(caller!=NULL);

	if(!component){
		stack = vnext(stack);
		assert(VERROR(stack));
		return stack;
	}
	if(component->state==NOFOUND)
		return setError(stack, L"Error in component_evaluate(): component->state==NOFOUND.");
	assert(c_oper(component)!=NULL);

	// see struct OperEval inside expression.h
	memset(stack, 0, sizeof(OperEval));
	((OperEval*)stack)->caller = caller;

	// prepare for a custom SET_VAR_FUNC operation
	value oper = stack + OperEvalSize;
	value v = oper;
	*(const_Str3*)(v+1) = c_name(component);
	*(Component**)(v+1+4) = component;
	v = setOpers(v, SET_VAR_FUNC, 4+2);
	v = setOpers(v, 0, 0);

	// set argument to the SET_VAR_FUNC operation
	if(!argument) v = setEmptyVector(v);
	else v = setAbsRef(v, vGet(argument));

	// relative offset to start of evaluation
	((OperEval*)stack)->start = v - stack - OperEvalSize;

	return operations_evaluate(stack, oper);
}

/**************************************************************************************************************/


static void replace_text (Str3 from, Str3 to)
{
	assert(from.end!=NULL);
	assert(to.end==NULL);
	if(strEnd3(from) || strEnd3(to)) return; // empty strings are not allowed
	to = setEnd(to);

	lchar* chr = to.end;
	to.end = chr->prev; // skip char '\0'
	chr->prev=NULL; str3_free(C37(chr)); // free '\0'

	sChar(from) = sChar(to); // copy 1st char
	chr = to.ptr; // prepare to free 1st char
	bool b = to.end!=to.ptr;
	if(b) to.ptr = chr->next; // skip 1st char
	chr->next=NULL; str3_free(C37(chr)); // free 1st char

	chr = from.ptr->next; // skip 1st char
	if(chr != from.end)
	{
		from.end->prev->next = NULL; // prepare to free lchar
		chr->prev = NULL; str3_free(C37(chr));
	}
	from.ptr->next = b ? to.ptr : from.end;
	if(b) to.ptr->prev = from.ptr;

	from.end->prev = b ? to.end : from.ptr;
	if(b) to.end->next = from.end;
}


static long _evaluation_instance = 0;
long evaluation_instance (bool increment)
{
	if(increment) _evaluation_instance++;
	return _evaluation_instance;
}

void replacement_record (const_value repl)
{
	Component *c;
	memcpy(&c, repl+1+4, sizeof(c));
	c = c_container(c);
	tree_do(
		TREE_ADD,
		&c->replacement,
		&repl,
		sizeof(repl),
		NULL,
		pointer_compare);
}


long replacement (value stack, Container *c, enum REPL_OPERATION opr)
{
	if(c==NULL || c->replacement.size==0) return 0;
	if(opr==REPL_COUNT) return c->replacement.size;
	if(opr==REPL_CANCEL) tree_free(&c->replacement);
	if(opr!=REPL_COMMIT) return 0;

	bool exists = list_find(container_list(), NULL, pointer_compare, &c);
	assert(exists); if(!exists) return 0;

	void* node = tree_first(&c->replacement);
	for( ; node != NULL; node = tree_next(node))
	{
		const_value repl = *(const_value*)node;
		assert(repl!=NULL); if(repl==NULL) continue;

		Str3 from;
		memcpy(&from, repl+1, sizeof(from));
		assert(!strEnd3(from) && from.end);

		// Locate the left-hand-side to be replaced
		while(isSpace(from.ptr->chr.c      )) from.ptr = from.ptr->next;
		while(isSpace(from.end->prev->chr.c)) from.end = from.end->prev;
		assert(!strEnd3(from)); if(strEnd3(from)) continue;

		// Get the string to replace with
		repl -= repl[1+4+2+2];
		stack = VstToStr(setRef(stack, repl), TOSTR_ESCAPE);
		Str3 to = astrcpy32(C37(0), getStr2(vGetPrev(stack)), NULL);

		// Finally perform the replacement
		replace_text(from, to);
	}
	tree_free(&c->replacement);
	return 1;
}

/**************************************************************************************************************/


// main global variables
static size_t _stackSize = 0;
static value _stackArray = 0;

size_t stackSize() { return _stackSize; }
value stackArray() { return _stackArray; }

void rfet_init (size_t stack_size)
{
	if(stack_size)
	{
		if(stack_size < 50000) stack_size = 50000;

		void* mem = _realloc (_stackArray, stack_size*sizeof(*_stackArray), "stackArray");
		if(mem){
			_stackSize = stack_size;
			_stackArray = (value)mem;
		}
		operations_init(stackArray());
	}
	else // do rfet_clean()
	{
		// note: line below must come first
		component_destroy(rootContainer);
		rootContainer = NULL;
		_free(_stackArray, "stackArray");
		_stackArray=NULL;
		string_clean();
	}
}

