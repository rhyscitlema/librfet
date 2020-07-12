#ifndef _RFET_H
#define _RFET_H
/*
	rfet.h
*/

#include <_stddef.h>

typedef void* RFET;


/* Parse the given <text> expression string.
*  On failure keep current <rfet> and return NULL.
*  NOTE: always set text=NULL directly after call.
*/
RFET rfet_parse (value stack, RFET rfet, Str3 text);


/* Evaluate multiple times on different arguments
*  or different outsider-values like 'time'.
*/
value rfet_evaluate (value stack, RFET rfet, const_value argument);


/* Commit changes to the rfet_container, return false if there is none */
bool rfet_commit_replacement (value stack, RFET rfet);

/* Get the rfet_container; often used upon if(rfet_commit_replacement()) */
value rfet_get_container_text (value stack, RFET rfet);

/* Free all resources allocated */
bool rfet_remove (value stack, RFET rfet);


#ifndef __STRING_H
/*
	The below is defined only in librfet.
	It is a once-off evaluation.
	Typical usage is:

	stack = rfet_parse_and_evaluate (stack, rfet_string, NULL, NULL);
	stack = VstToStr(stack, TOSTR_NEWLINE); // see _math.h
	puts2(getStr2(vGetPrev(stack))); // print output string to stdout
*/
value rfet_parse_and_evaluate (
	value stack,
	const_Str2 rfet_string,     /* string to be parsed and evaluated */
	const_Str2 source_name,     /* source of rfet_string, if NULL then "input" is used */
	const_value argument );
#endif


int execute (int argc, char** argv, int verbose); // see main.c

#endif

