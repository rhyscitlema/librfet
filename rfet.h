#ifndef _RFET_H
#define _RFET_H
/*
    rfet.h
*/

#include <_stddef.h>

typedef void* RFET;


/* Parsing of the given expression string.
   Returns NULL on failure, and if 'rfet'
   was not NULL then it is not removed.
   NOTE: directly after call, set text=NULL.
*/
extern RFET rfet_parse (RFET rfet, lchar* text);

/* Evaluate multiple times using different arguments or on different values of time 't' */
extern bool rfet_evaluate (RFET rfet, const value* argument, const value* result_vst);

/* Commit changes to the rfet_container, return false if there is none */
extern bool rfet_commit_replacement (RFET rfet);

/* Get the rfet_container, especially if it has changed */
extern const wchar* rfet_get_container_text (RFET rfet);

/* Free all resources allocated */
extern void rfet_remove (RFET rfet);


#ifndef __STRING_H
/*
    The below is defined only in librfet.
    It is a once-off evaluation.
    Returns non-NULL on success.
    Typical usage is:

    const value* result = rfet_parse_and_evaluate(input_string, NULL);

    if(result==NULL) puts2(errorMessage());

    else if(VstToStr(result, output_string, 4, -1, -1, -1)) // see _math.h

         puts2(output_string); // output_string contains the VstToStr

    else puts2(output_string); // output_string contains the error message
*/
extern const value* rfet_parse_and_evaluate (
    const wchar* rfet_string,   /* string to be parsed and evaluated */
    const wchar* source_name,   /* source of rfet_string, if NULL then "input" is used */
    const value* result_vst);   /* the 'expected' result value structure, NULL for any */
#endif


#endif

