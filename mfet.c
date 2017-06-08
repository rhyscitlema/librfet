/*
    mfet.c
*/

#include <mfet.h>
#include <expression.h>
#include <component.h>
#include <_texts.h>
#include <_stdio.h>

/******************************************************************************/

const value* mfet_parse_and_evaluate (
    const wchar* mfet_string,
    const wchar* source_name,
    const value* result_vst)
{
    wchar source[10];
    strcpy21(source, "input");
    if(!source_name) source_name = source;

    lchar* text=NULL; astrcpy32(&text, mfet_string);
    set_line_coln_source(text, 1, 1, source_name);

    MFET mfet = mfet_parse(NULL, text); text=NULL;
    bool success = mfet_evaluate(mfet, NULL, result_vst);

    mfet_remove(mfet);
    return success ? mainStack() : NULL;
}


/* check for container and return true if it exists */
static bool container_check (const Container* container)
{
    if(!container) return false;
    if(list_find(containers_list, NULL, pointer_compare, &container)) return true;
    sprintf2(errorMessage(), L"MFET Error: container cannot be found.\r\n");
    return false;
}

/******************************************************************************/

MFET mfet_parse (MFET mfet, lchar* text)
{
    if(mfet && !container_check((Container*)mfet)) return false;

    if(isEmpty3(text,0))
    {   strcpy21(errorMessage(), "Empty input.");
        lchar_free(text);
        return NULL;
    }
    lchar* name=NULL;
    astrcpy32(&name, lchar_get_source(*text));

    Container* parent = mfet ? ((Container*)mfet)->parent : NULL;
    Container* container = container_parse(parent, name, text); text=NULL;
    if(container && !dependence_parse()) container=NULL;
    dependence_finalise(container!=NULL);

    return container;
}


bool mfet_evaluate (MFET mfet, const value* argument, const value* result_vst)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return false;

    GeneralArg garg = {
        .caller = (Container*)mfet,
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
    return component_evaluate(eca, (Component*)mfet, result_vst);
}


bool mfet_commit_replacement (MFET mfet)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return false;
    if(container->replacement_count==0) return false;
    replacement_commit(container);
    return true;
}


const wchar* mfet_get_container_text (MFET mfet)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return NULL;
    return CST23(c_mfet(container));
}


void mfet_remove (MFET mfet)
{ if(mfet) inherits_remove((Container*)mfet); }

