/*
	outsider_default.c

	Note:
	I am needed by librfet.
	There is no need to compile me independently,
	just #include me anywhere in main.c file.
	I am essentially just a 'template'.
	By default, the only outsider I have is 'time'.
*/

#include <outsider.h>

bool user_input_allowed = false;


enum OUTSIDER_ID
{
	NONE=0,
	TIME,
	END
};


/* Get Outsider ID */
#define GOID(string, ID) if(0==strcmp31(str, string)) return ID;


/* Return outsider's ID != 0, else return 0 */
int outsider_getID (const_Str3 str)
{
	GOID("time", TIME)
	return NONE;
}


/* Evaluate outsider expression */
value set_outsider (value stack, int ID)
{
	switch(ID)
	{
	case TIME:
		stack = setSmaInt(stack, 0);
		break;

	default:{
		const_Str2 argv[2];
		argv[0] = L"Software outsider ID = %s is unrecognised.";
		argv[1] = TIS2(0,ID);
		stack = setMessage(stack, 0, 2, argv);
		}break;
	}
	return stack;
}


/* Called by container_parse() in component.c */
bool checkfail (value stack, const Container* c, const_Str3 name, bool hasType, bool isNew)
{ return (stack || c || name.ptr || hasType || isNew) ? false : false; }

