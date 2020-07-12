/*
	main.c
*/

#include <stdio.h>
#include <_stdio.h>
#include "rfet.h"
#include "component.h"
#include "outsider_default.c"


#ifndef SKIP_MAIN

void user_alert (const wchar* title, const wchar* message)
{
	#ifdef LOCAL_USER
	printf("\r\n>>> "); puts2(title); puts2(message);
	printf("Press Enter to continue..."); getchar();
	#endif
}

bool user_confirm (const wchar* title, const wchar* message)
{
	#ifdef LOCAL_USER
	char buffer[10];
	printf("\r\n>>> "); puts2(title); puts2(message);
	printf("Enter ok / <anything> : ");
	if(!fgets(buffer, sizeof(buffer), stdin))
		return false;
	else return 0==strcmp(buffer, "ok\n");
	#else
	return false;
	#endif
}

const wchar* user_prompt (const wchar* title, const wchar* message, const wchar* entry)
{
	#ifdef LOCAL_USER
	char buffer[100000];
	printf("\r\n>>> "); puts2(title); puts2(message);
	printf("\r\nEntry: "); puts2(entry);
	printf("Reply: ");

	if(!fgets(buffer, sizeof(buffer), stdin))
		strcpy12(buffer, entry);
	else{
		size_t len = strlen(buffer);
		buffer[len-1] = '\0'; // skip the '\n' at the end
	}
	return CST21(buffer);
	#else
	return NULL;
	#endif
}

int main (int argc, char** argv)
{
	int verbose = execute(argc, argv, 1);
	if(verbose>=2) printf("\r\n");
	if(verbose>=3) component_print(" |  ", 0, container_find(0,0,C37(NULL),0));
	rfet_clean();
	if(verbose>=2) memory_print();
	return 0;
}

#endif


static Container* current = NULL;

static void tools_select (Container* container, int show)
{
	if(!container) { puts1(""); return; }
	const_Str3 rfetStr = c_rfet(container);
	assert(!strEnd3(rfetStr));

	void* node = list_find(container_list(), NULL, pointer_compare, &container);
	assert(node!=NULL); if(node==NULL) return;
	list_head_push(container_list(), list_node_pop(container_list(), node));

	if(show>=2){
		puts("________________________________________");
		puts3(rfetStr);
	}
	if(show>=1){
		uint32_t stack[1000];
		puts2(container_path_name(stack, container));
	}
	current = container;
}

static void tools_set_path (const char* s, int show)
{
	const wchar* wstr = C21(s);
	long len = strlen2(wstr);
	lchar lstr[len+1];
	Str3 path = set_lchar_array(lstr, len+1, wstr, L"path name");

	value stack = stackArray();
	Container* container = NULL;
	if(wstr && *wstr && *wstr!='|')
		setError(stack, L"Error: path name must start with a '|'.");
	else container = container_find(stack, NULL, path, true);

	if(!container) puts2(getMessage(vGet(stack)));
	else if(!container->name1.ptr)
		current = NULL; // if rootContainer
	else if(!strEnd3(c_rfet(container)))
		tools_select(container, show);
}

static void tools_get_prev (int show)
{
	void* node = list_head_pop(container_list());
	list_tail_push(container_list(), node);
	node = list_head(container_list());
	if(node) tools_select(*(Container**)node, show);
}

static void tools_get_next (int show)
{
	void* node = list_tail_pop(container_list());
	list_head_push(container_list(), node);
	if(node) tools_select(*(Container**)node, show);
}


int execute (int argc, char** argv, int verbose)
{
	bool scanning = false;
	char entry[1000]="", *s;
	rfet_init(1000000);

	const char* info = "Enter <rfet> or :command or :help or :quit";
	if(argc==1) { scanning=true; puts(info); }

	int i=0; // 'i' indexes the 'argv' array
	while(true)
	{
		if(scanning)
		{
			if(verbose>=1) printf("(rfet) ");
			char str[sizeof(entry)];

			if(!fgets(str, sizeof(str), stdin)
			|| 0==strcmp(str, ":q\n"))
				strcpy(str, ":quit\n");

			if(*str=='\n' || *str=='\0') // if an empty user entry
				s = entry;
			else {
				str[strlen(str)-1] = 0; // remove '\n' from end of string
				strcpy(entry, str); // must come after
				s = entry;
			}
		}
		else if(++i >= argc) break;
		else s = argv[i];

		value stack = stackArray();
		const wchar* source = NULL;
		Container* trfet = NULL;

		     if(0==strcmp(s, ":v++")) { printf("verbose = %d\r\n", ++verbose); }
		else if(0==strcmp(s, ":v--")) { printf("verbose = %d\r\n", --verbose); }
		else if(0==strcmp(s, ":scan")) { scanning=true; strcpy(entry,s); puts(info); }
		else if(0==strcmp(s, ":quit")) { scanning=false; strcpy(entry,s); }
		else if(0==strcmp(s, ":eval")) trfet = current;
		else if(0==strcmp(s, ":rfet")) tools_select(current, 2);
		else if(0==strcmp(s, ":path")) tools_select(current, 1);
		else if(0==strcmp(s, ":prev")) tools_get_prev(verbose);
		else if(0==strcmp(s, ":next")) tools_get_next(verbose);
		else if(0==memcmp(s, ":path=", 6)) tools_set_path(s+6, verbose);
		else if(0==memcmp(s, ":file=", 6))
		{
			const_Str2 filename = C21(s+6);
			if(VERROR(FileOpen2(filename, stack)))
			{
				puts2(getMessage(stack));
				if(!scanning) break;
			}
			else source = get_name_from_path_name(filename);
		}
		else if(0==strcmp(s, ":dele"))
		{
			if(current && inherits_remove(current)){
				current = NULL;
				if(verbose>=1) printf("Container deleted.\r\n");
			}
		}
		else if(0==strcmp(s, ":parse"))
		{
			if(current){
				setStr23(stack, c_rfet(current)); // copy string
				source = C23(c_name(current));
			}
		}
		else if(0==strcmp(s, ":help"))
		{
			printf("\r\n"
			   "The following are valid program calls:\r\n"
			);
		}
		else if(s[0]==':') printf("Error: invalid command \"%s\".\r\n", s);
		else
		{
			setStr21(stack, s); // copy string
			if(!current) source = L"input";
			else source = C23(c_name(current));
		}

		if(!source && !trfet) continue;
		if(source)
		{
			Str3 text = astrcpy32(C37(NULL), getStr2(stack), source);
			trfet = (Container*)rfet_parse(stack, current, text); text=C37(NULL);
		}

		if(!trfet) { stack = vnext(stack); assert(VERROR(stack)); }
		else{
			tools_select(trfet, 0); // sets current = trfet
			stack = rfet_evaluate(stack, trfet, NULL);

			if(VERROR(stack)) trfet=NULL;
			else{
				stack = VstToStr(stack, TOSTR_NEWLINE); // see _math.h

				if(rfet_commit_replacement(stack, trfet) && verbose>=1)
				{
					stack = setStr21(stack, "\r\n----------INPUT-CHANGED----------\r\n");
					stack = rfet_get_container_text(stack, trfet);
					stack = _add(_add(stack)); // concatenate strings
				}
			}
		}
		const_Str2 out;
		if(trfet) out = getStr2(vGetPrev(stack)); // get final result
		else out = getMessage(vGetPrev(stack)); // get error message
		puts1(C12(out)); // print result to stdout
		if(!trfet && !scanning) break;
	}
	return verbose;
}
