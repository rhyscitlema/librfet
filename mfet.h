#ifndef _MFET_H
#define _MFET_H
/*
    mfet.h
*/

#include <_stdint.h>

typedef void* MFET;


/* Parsing of the given expression string. Returns success
   with *mfet_ptr changed, or failure with *mfet_ptr unchanged.
   IMPORTANT: set *mfet_ptr = NULL when parsing for the first time.
*/
extern bool mfet_parse (
    MFET* mfet_ptr,                     /* (*mfet_ptr = NULL) when parsing for the first time */
    const mchar* parameter,             /* = "(?,?,?)" != NULL if expression is a function */
    const mchar* mfet_container,        /* = String to be parsed */
    const mchar* source_name,           /* = Name of source of mfet_container. If NULL then the default is used */
    int start_line, int start_column);  /* = Starting line and column of expression_string; by default, provide 1,1. */


/* Evaluate multiple times using different arguments or on different values of time 't' */
extern bool mfet_evaluate (MFET mfet, const Value* arguments);

/* Commit changes to the mfet_container, return false if there is none */
extern bool mfet_commit_replacement (MFET mfet);

/* Get the mfet_container, especially if it has changed */
extern const mchar* mfet_get_mfet_container (MFET mfet);

/* Get the result of evaluation as a string */
extern const mchar* mfet_get_result_string (MFET mfet);

/* Free all resources allocated */
extern void mfet_remove (MFET mfet);

/* Get the error message of the latest failure */
#define mfet_get_error_message(mfet) error_message


#endif

