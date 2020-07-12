/*
	rfet.c
*/

#include "rfet.h"
#include "component.h"


value rfet_parse_and_evaluate (
	value stack,
	const_Str2 rfet_string,
	const_Str2 source_name,
	const_value argument )
{
	if(strEnd2(source_name)) source_name = L"input";
	Str3 text = astrcpy32(C37(NULL), rfet_string, source_name);

	RFET rfet = rfet_parse(stack, NULL, text); text=C37(NULL);
	stack = rfet_evaluate(stack, rfet, argument);

	if(!rfet) assert(VERROR(stack));
	rfet_remove(stack, rfet);
	return stack;
}


/* check if container exists and return true if it does */
static bool container_check (value stack, const Container* container)
{
	if(!container) return false;
	if(list_find(container_list(), NULL, pointer_compare, &container)) return true;
	setError(stack, L"RFET Error: container cannot be found.");
	assert(!"container_check() failed.");
	return false;
}


RFET rfet_parse (value stack, RFET rfet, Str3 text)
{
	if(rfet && !container_check(stack, (Container*)rfet))
	{ text = str3_free(text); return NULL; }

	if(strEmpty3(text,0))
	{
		stack = setError(stack, L"Empty input.");
		text = str3_free(text); return NULL;
	}
	assert(text.end==NULL); // assert is a mallocated Str3
	Str3 name = astrcpy32(C37(NULL), lchar_get_source(text), NULL);

	Container* parent = rfet ? ((Container*)rfet)->parent : NULL;
	Container* container = container_parse(stack, parent, name, text);
	name=C37(NULL); text=C37(NULL);

	if(container && !dependence_parse(stack)) container=NULL;
	dependence_finalise(container!=NULL);

	return container;
}


value rfet_evaluate (value stack, RFET rfet, const_value argument)
{
	Container *container = (Container*)rfet;
	if(!container_check(stack, container)) return vnext(stack);
	evaluation_instance(true);
	return component_evaluate (stack, container, container, argument);
}

bool rfet_commit_replacement (value stack, RFET rfet)
{
	Container *container = (Container*)rfet;
	if(!container_check(stack, container)) return false;
	return replacement(stack, container, REPL_COMMIT);
}

value rfet_get_container_text (value stack, RFET rfet)
{
	Container *container = (Container*)rfet;
	if(!container_check(stack, container)) return vnext(stack);
	return setStr23(stack, c_rfet(container));
}

bool rfet_remove (value stack, RFET rfet)
{
	Container *container = (Container*)rfet;
	if(!container_check(stack, container)) return false;
	return inherits_remove((Container*)rfet);
}

