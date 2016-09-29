
/*
    opr_images.c

    This file is #included by operations.c only.
*/

#ifdef RWIF

#include <_string.h>
#include <rwif.h>
#include <malloc.h> // used by free()



typedef struct _Image
{   mchar fileName[300];
    ImageData imagedata;
} Image;

#define MAX 100
static Image ImageArray[MAX];



inline static unsigned int checkID (unsigned int ID)
{
    if(ID==0) return MAX;
    ID--;
    if(ID >= MAX)
    {
        strcpy21 (error_message, "Software Error: ID of image file is invalid. Please report this!");
        return MAX;
    }
    if(ImageArray[ID].fileName[0]==0)
    {
        strcpy21 (error_message, "Error: image file not loaded.");
        return MAX;
    }
    return ID;
}



/* free all resources used to store image */
/*static*/ void unloadImageFile (unsigned int ID)
{
    ID=checkID(ID); if(ID>=MAX) return;
    free(ImageArray[ID].imagedata.pixelArray);
    ImageArray[ID].fileName[0]=0;
}



/* on success: return ID of loaded image file. ID > 0.
 * on failure: update error_message and return 0. */
static unsigned int loadImageFile (const mchar* fileName)
{
    unsigned int i, ID;
    const char* filename;

    if(fileName==NULL) { strcpy21 (error_message, "Software Error: In loadImageFile(): No file name specified. Please report this!"); return 0; }

    i=MAX;
    for(ID=0; ID<MAX; ID++)
    {
        /* look for the first free slot */
        if(i==MAX && ImageArray[ID].fileName[0]==0) i = ID;

        /* check if file was already loaded */
        if(0==strcmp22 (ImageArray[ID].fileName, fileName)) return ID+1;
    }
    if(i==MAX) { sprintf2 (error_message, CST21("Error: Cannot load more than %s files."), TIS2(0,MAX)); return 0; }
    ID = i;

    filename = add_path_to_file_name (fileName,0);

    if(!read_image_file (filename, &ImageArray[ID].imagedata))
    {
        strcpy21(error_message, rwif_error_message);
        return 0;
    }
    strcpy22 (ImageArray[ID].fileName, fileName);
    return ID+1;
}



static bool load_image_file (Expression* expression, const lchar* lstr)
{
    unsigned int ID;
    mchar name[300];

    /* check if there was a successfull loading before */
    if(expression->info > 0) return true;

    /* check if there was an unsuccessfull loading before */
    if(expression->info < 0) return false;

    /* else this is the first time to enter this function */
    strcpy23(name, lstr);
    ID = loadImageFile(name);

    if(ID==0)   /* if failure to load image */
         { expression->info = -1; return false; }
    else { expression->info = ID; return true; }
}



Value* get_image_width_height (Expression* expression, const Value* argument)
{
    unsigned int ID = expression->info;
    Value* out=NULL;

    if(ID==0) // if first time
    {
        Value* in = expression_evaluate(expression->headChild, argument);
        if(in==NULL);
        else if(!isString(*in))
            set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nArgument must evaluate to a file name."), expression->name, 0, 0);
        else if(load_image_file (expression, getString(*in)))
            ID = expression->info;
        value_free(in);
    }
    if(ID>0)
    {   ID--;
        out = value_alloc(3);
        setSepto2 (out[0], 3);
        setSmaInt (out[1], ImageArray[ID].imagedata.width);
        setSmaInt (out[2], ImageArray[ID].imagedata.height);
        expression->independent = 1;
        INDEPENDENT(expression)
    }
    return out;
}



Value* get_image_pixel_colour (Expression* expression, const Value* argument)
{
    unsigned int ID;
    long x, y, w, h, B;
    const unsigned char* colour;
    Value* out=NULL;

    Value* in = expression_evaluate(expression->headChild, argument);

    ID = expression->info;
    if(ID==0) // if first time
    {
        if(!check_first_level(in, 3, expression->name));

        else if(!isString(in[1]))
            set_error_message (CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nFirst argument must evaluate to a file name."), expression->name);

        else if(load_image_file (expression, getString(in[1])))
            ID = expression->info;
    }
    if(ID>0)
    {   ID--;

        colour = ImageArray[ID].imagedata.pixelArray;
        h      = ImageArray[ID].imagedata.height;
        w      = ImageArray[ID].imagedata.width;
        B      = ImageArray[ID].imagedata.bpp/8;

        x = toSmaInt(in[2],0); x %= w; if(x<0) x += w;
        y = toSmaInt(in[3],0); y %= h; if(y<0) y += h;
        colour += (y*w + x)*B;

        out = value_alloc(5);
        setSepto2 (out[0+0], 5);
        setSmaRa2 (out[1+0], colour[0], 255);
        setSmaRa2 (out[1+1], colour[1], 255);
        setSmaRa2 (out[1+2], colour[2], 255);
        setSmaRa2 (out[1+3], (B==4 ? colour[3] : 255), 255);
    }
    value_free(in);
    return out;
}



Value* get_image_pixel_matrix (Expression* expression, const Value* argument)
{
    NOT_YET
}



#else // else of #ifdef RWIF

#include <_string.h>
Value* get_image_width_height (Expression* expression, const Value* argument) NOT_YET
Value* get_image_pixel_colour (Expression* expression, const Value* argument) NOT_YET
Value* get_image_pixel_matrix (Expression* expression, const Value* argument) NOT_YET

#endif // end of #ifdef RWIF

