/*
    main.c
*/

#include <_stdio.h>
#include <_string.h>
#include <_texts.h>
#include "rfet.h"
#include "component.h"
#include "outsider_default.c"

enum TYPE {
    None,
    Error,
    Enter,
    Text,
    File
};


void wait_for_user_first (const wchar* title, const wchar* message)
{
    printf("\r\n>>>Title: "); puts2(title);
    printf("\r\n>>>Message:\r\n"); puts2(message);
    printf("\r\nPress Enter to continue..."); getchar();
}

bool wait_for_confirmation (const wchar* title, const wchar* message)
{
    char buffer[1000];
    printf("\r\n>>>Title: "); puts2(title);
    printf(">>>Message:\r\n"); puts2(message);
    printf("Enter 1 for 'YES' or anything else for 'NO' : ");
    if(!fgets(buffer, sizeof(buffer), stdin)) return false;
    return (buffer[0]=='1' && (buffer[1]=='\n' || buffer[1]==0));
}


int main (int argc, char** argv)
{
    int verbose=0;
    char entry[1000];
    const wchar* source_name = NULL;
    Array2 input = {0};
    lchar* text = NULL;
    RFET rfet = NULL;
    rfet_init(1000000);

    enum TYPE type = (argc==1) ? Enter : None;
    int i=0;
    while(true)
    {
        if(type!=Enter) { if(++i >= argc) break; }

             if(0==strcmp(argv[i], "-v")) { verbose++; continue; }
        else if(0==strcmp(argv[i], "-e")) { type = Enter; }
        else if(0==strcmp(argv[i], "-t")) { type = Text; continue; }
        else if(0==strcmp(argv[i], "-f")) { type = File; continue; }
        else if(type==None) { type = Error; break; }

        if(type==Enter)
        {
            while(true)
            {
                printf("Enter RFET (q to quit): ");
                if(!fgets(entry, sizeof(entry), stdin)) continue;

                if(entry[0]=='\n' || entry[0]=='\0') continue;
                if(entry[0]=='q') { type = None; break; }

                entry[strlen(entry)-1] = 0;     // remove '\n' from end of string
                astrcpy21 (&input.data, entry); // copy string

                source_name = L"input";
                break;
            }
            if(type==None) continue;
        }
        else if(type==Text)
        {
            if(verbose) printf("-----------------------------\r\n");
            source_name = L"input";
            astrcpy21 (&input.data, argv[i]);
        }
        else if(type==File)
        {
            if(verbose) printf("_____________________________\r\n");
            source_name = get_name_from_path_name(NULL,CST21(argv[i]));

            input.size=0;
            if((input = FileOpen2(source_name, input)).size<1)
            {
                printf("Error: file '%s' could not be opened.\r\n", argv[2]);
                break;
            }
        }

        astrcpy32(&text, input.data);
        set_line_coln_source(text, 1, 1, source_name);

        RFET trfet = rfet_parse(rfet, text); text=NULL;
        if(trfet) rfet = trfet;
        if(rfet_evaluate(trfet, NULL, NULL))
            VstToStr(mainStack(), errorMessage(), 4, -1, -1, -1);
            // put result inside the errorMessage global string

        puts2(errorMessage());
    }

    if(type==Error)
    {
        printf("\r\n"
               "The following are valid program calls:\r\n"
               "\r\n"
               "   1) <program> -e\r\n"
               "      - The RFET is later entered then evaluated\r\n"
               "\r\n"
               "   2) <program> -t <rfet>\r\n"
               "      - The given text <rfet> is evaluated\r\n"
               "\r\n"
               "   3) <program> -f <name>\r\n"
               "      - The given file <name> is evaluated\r\n"
               "\r\n"
               "   Example: <program> -t '1+1' -f example.rfet\r\n"
               "\r\n");
        for(i=0; i<argc; i++) printf("Argument[%d] = %s\r\n", i, argv[i]);
        printf("\r\n");
        return 0;
    }

    if(verbose) printf("\r\n");
    if(verbose==2) component_print(" |  ", 0, container_find(0,0,0,0,0));
    rfet_clean();
    lchar_free(text);
    wchar_free(input.data);
    if(verbose) memory_print();
    return 0;
}

