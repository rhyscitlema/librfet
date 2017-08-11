/*
    rfet.c
*/

#include <rfet.h>
#include <expression.h>
#include <component.h>
#include <_texts.h>
#include <_stdio.h>

/******************************************************************************/

const value* rfet_parse_and_evaluate (
    const wchar* rfet_string,
    const wchar* source_name,
    const value* result_vst)
{
    wchar source[10];
    strcpy21(source, "input");
    if(!source_name) source_name = source;

    lchar* text=NULL; astrcpy32(&text, rfet_string);
    set_line_coln_source(text, 1, 1, source_name);

    RFET rfet = rfet_parse(NULL, text); text=NULL;
    bool success = rfet_evaluate(rfet, NULL, result_vst);

    rfet_remove(rfet);
    return success ? mainStack() : NULL;
}


/* check for container and return true if it exists */
static bool container_check (const Container* container)
{
    if(!container) return false;
    if(list_find(containers_list, NULL, pointer_compare, &container)) return true;
    sprintf2(errorMessage(), L"RFET Error: container cannot be found.\r\n");
    return false;
}

/******************************************************************************/

RFET rfet_parse (RFET rfet, lchar* text)
{
    if(rfet && !container_check((Container*)rfet)) return false;

    if(isEmpty3(text,0))
    {   strcpy21(errorMessage(), "Empty input.");
        lchar_free(text);
        return NULL;
    }
    lchar* name=NULL;
    astrcpy32(&name, lchar_get_source(*text));

    Container* parent = rfet ? ((Container*)rfet)->parent : NULL;
    Container* container = container_parse(parent, name, text); text=NULL;
    if(container && !dependence_parse()) container=NULL;
    dependence_finalise(container!=NULL);

    return container;
}


bool rfet_evaluate (RFET rfet, const value* argument, const value* result_vst)
{
    Container *container = (Container*)rfet;
    if(!container_check(container)) return false;

    GeneralArg garg = {
        .caller = (Container*)rfet,
        .message = errorMessage(),
        .argument = argument
    };
    ExprCallArg eca = {
        .garg = &garg,
        .expression = NULL,
        .stack = mainStack()
    };
    evaluation_instance++;
    replacement_container = container;
    return component_evaluate(eca, (Component*)rfet, result_vst);
}


bool rfet_commit_replacement (RFET rfet)
{
    Container *container = (Container*)rfet;
    if(!container_check(container)) return false;
    if(container->replacement_count==0) return false;
    replacement_commit(container);
    return true;
}


const wchar* rfet_get_container_text (RFET rfet)
{
    Container *container = (Container*)rfet;
    if(!container_check(container)) return NULL;
    return CST23(c_rfet(container));
}


void rfet_remove (RFET rfet)
{ if(rfet) inherits_remove((Container*)rfet); }

