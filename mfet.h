#ifndef _MFET_H
#define _MFET_H
/*
    mfet.h
*/

#include <_stddef.h>

typedef void* MFET;


/* Parsing of the given expression string.
   Returns NULL on failure, and if 'mfet'
   was not NULL then it is not removed.
   NOTE: directly after call, set text=NULL.
*/
extern MFET mfet_parse (MFET mfet, lchar* text);

/* Evaluate multiple times using different arguments or on different values of time 't' */
extern bool mfet_evaluate (MFET mfet, const value* argument, const value* result_vst);

/* Commit changes to the mfet_container, return false if there is none */
extern bool mfet_commit_replacement (MFET mfet);

/* Get the mfet_container, especially if it has changed */
extern const wchar* mfet_get_container_text (MFET mfet);

/* Free all resources allocated */
extern void mfet_remove (MFET mfet);


#ifndef __STRING_H
/*
    The below is defined only in libmfet.
    It is a once-off evaluation.
    Returns non-NULL on success.
    Typical usage is:

    const value* result = mfet_parse_and_evaluate(input_string, NULL);

    if(result==NULL) puts2(errorMessage());

    else if(VstToStr(result, output_string, 4, -1, -1, -1)) // see _math.h

         puts2(output_string); // output_string contains the VstToStr

    else puts2(output_string); // output_string contains the error message
*/
extern const value* mfet_parse_and_evaluate (
    const wchar* mfet_string,   /* string to be parsed and evaluated */
    const wchar* source_name,   /* source of mfet_string, if NULL then "input" is used */
    const value* result_vst);   /* the 'expected' result value structure, NULL for any */
#endif


#endif

