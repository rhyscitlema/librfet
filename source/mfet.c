/*
    mfet.c
*/

#include <mfet.h>
#include <expression.h>
#include <component.h>
#include <_texts.h>
#include <_stdio.h>

/******************************************************************************/

const mchar* mfet_parse_and_evaluate (
    const mchar* mfet_container,
    const mchar* source_name)
{
    const mchar* result=NULL;
    MFET mfet=NULL;

    if(mfet_parse(&mfet, NULL, mfet_container, source_name, 1, 1)
    && mfet_evaluate(mfet, NULL))
        result = mfet_get_result_string(mfet);

    mfet_remove(mfet);
    return result;
}

bool mfet_to_integer (int *n, const mchar* entry, const mchar* source)
{
    const mchar* str;
    if(isEmpty2(entry,0)) { strcpy21(error_message, "No entry."); return false; }
    str = mfet_parse_and_evaluate(entry, source); if(str==NULL) return false;
    if(!strToInt(str,n)) { sprintf2(error_message, CST21("%s is not a valid integer."), str); return false; }
    return true;
}

bool mfet_to_float (SmaFlt *n, const mchar* entry, const mchar* source)
{
    const mchar* str;
    if(isEmpty2(entry,0)) { strcpy21(error_message, "No entry."); return false; }
    str = mfet_parse_and_evaluate(entry, source); if(str==NULL) return false;
    if(!strToFlt(str,n)) { sprintf2(error_message, CST21("%s is not a valid number."), str); return false; }
    return true;
}

/******************************************************************************/

bool mfet_parse (
    MFET* mfet_ptr,
    const mchar* parameter,
    const mchar* mfet_container,
    const mchar* source_name,
    int start_line, int start_column)
{
    bool success = false;
    lchar* strExpr=NULL;
    Container *container;

    if(mfet_ptr==NULL || mfet_container==NULL) return false;

    // get and setup the input
    astrcpy32 (&strExpr, mfet_container);
    set_line_coln (strExpr, start_line, start_column);

    if(source_name==NULL) source_name = CST21("input");
    set_lchar_file (strExpr, submitSourceName(source_name));

    // parse with the 4 main and only steps
    container = (Container*)(*mfet_ptr);
    if(container_add(&container, NULL, strExpr)
    && dependence_parse()) success = true;
    dependence_finalise(success);

    if(success)
    {
        lchar_free(container->name); container->name = getmfetc(container, "name");
        lchar_free(container->type); container->type = getmfetc(container, "type");
        lchar_free(container->text); container->text = strExpr; strExpr = NULL;

        if(container->name==NULL)
        {   char* temp = (char*)error_message;
            sprintf(temp, "Calculator_%p", container);
            astrcpy31(&container->name, temp);
        }
        if(*mfet_ptr==NULL) *mfet_ptr = container;
    }
    else if(*mfet_ptr==NULL) // if container did not exists before
        container_remove(container);

    lchar_free(strExpr);
    return success;
}


bool mfet_evaluate (MFET mfet, const Value* arguments)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return false;
    if(container->mainc==NULL) { strcat21 (error_message, "\n\rError: container->mainc==NULL in mfet_evaluate()."); return false; }
    evaluation_instance++;
    replacement_container = container;
    Value* out = component_evaluate (container->mainc, arguments, 0);
    if(!out) return false;
    avaluecpy(&container->result, out);
    value_free(out);
    return true;
}


bool mfet_commit_replacement (MFET mfet)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return false;
    if(container->replacement_count==0) return false;
    replacement_commit(container);
    return true;
}


const mchar* mfet_get_mfet_container (MFET mfet)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return NULL;
    return CST23(container->text);
}


const mchar* mfet_get_result_string (MFET mfet)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return NULL;
    if(!VstToStr(container->result, error_message, 4, -1, -1, -1)) return NULL;
    return error_message;
}


void mfet_remove (MFET mfet)
{
    Container *container = (Container*)mfet;
    if(!container_check(container)) return;
    container_remove(container);
}

