#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define NS "stbimage"

/*
 * Only the _Init function is exported.
 */

extern DLLEXPORT int	Stbimage_Init(Tcl_Interp * interp);

/*
 * end block for C++
 */

#ifdef __cplusplus
}
#endif


static int tcl_stb_load(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
    char *filename = NULL;
    int len;
    int w = 0, h = 0, channels_in_file = 0;
    unsigned char *data = NULL;
    int length = 0;
    Tcl_Obj *result;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
    }

    filename = Tcl_GetStringFromObj(objv[1], &len);
    if (!filename || len < 1) {
        return TCL_ERROR;
    }

    data = stbi_load(filename, &w, &h, &channels_in_file, 0);
    if(!data) {
        if( interp ) {
            Tcl_Obj *resultObj = Tcl_GetObjResult( interp );
            Tcl_AppendStringsToObj( resultObj, "Load file failed",
                                    (char *)NULL );
        }

        return TCL_ERROR;
    }

    length = w * h * channels_in_file;
    result = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "width", -1 ), Tcl_NewIntObj( w ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "height", -1 ), Tcl_NewIntObj( h ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "channel", -1 ), Tcl_NewIntObj( channels_in_file ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "data", -1 ), Tcl_NewByteArrayObj( data, length));

    Tcl_SetObjResult(interp, result);
    stbi_image_free(data);
    return TCL_OK;
}


static int tcl_stb_load_from_memory(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
    unsigned char *filedata = NULL;
    int len;
    int w = 0, h = 0, channels_in_file = 0;
    unsigned char *data = NULL;
    int length = 0;
    Tcl_Obj *result;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "filedata");
        return TCL_ERROR;
    }

    filedata = Tcl_GetByteArrayFromObj(objv[1], &len);
    if (!filedata || len < 1) {
        return TCL_ERROR;
    }

    data = stbi_load_from_memory(filedata, len, &w, &h, &channels_in_file, 0);
    if(!data) {
        if( interp ) {
            Tcl_Obj *resultObj = Tcl_GetObjResult( interp );
            Tcl_AppendStringsToObj( resultObj, "Load file failed",
                                    (char *)NULL );
        }

        return TCL_ERROR;
    }

    length = w * h * channels_in_file;
    result = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "width", -1 ), Tcl_NewIntObj( w ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "height", -1 ), Tcl_NewIntObj( h ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "channel", -1 ), Tcl_NewIntObj( channels_in_file ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "data", -1 ), Tcl_NewByteArrayObj( data, length));

    Tcl_SetObjResult(interp, result);
    stbi_image_free(data);
    return TCL_OK;
}


static int tcl_stb_resize(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
    int in_w = 0, in_h = 0, out_w = 0, out_h = 0, num_channels = 0;
    unsigned char *input_pixels = NULL, *output_pixels = NULL;
    int len = 0;
    int res = 0;
    int length = 0;
    Tcl_Obj *result;

    if (objc != 7) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdata srcwidth srcheight dstwitdh dstheight num_channels");
        return TCL_ERROR;
    }

    input_pixels = Tcl_GetByteArrayFromObj(objv[1], &len);
    if (!input_pixels || len < 1) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[2], &in_w) != TCL_OK) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[3], &in_h) != TCL_OK) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[4], &out_w) != TCL_OK) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[5], &out_h) != TCL_OK) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[6], &num_channels) != TCL_OK) {
        return TCL_ERROR;
    }


    output_pixels = (unsigned char *) ckalloc (out_w * out_h * num_channels);
    if (!output_pixels) {
        return TCL_ERROR;
    }

    res = stbir_resize_uint8(input_pixels , in_w , in_h , 0,
                       output_pixels, out_w, out_h, 0,
                       num_channels);

    if (res == 0) {
        if( interp ) {
            Tcl_Obj *resultObj = Tcl_GetObjResult( interp );
            Tcl_AppendStringsToObj( resultObj, "Resize failed",
                                    (char *)NULL );
        }

        return TCL_ERROR;
    }

    length = out_w * out_h * num_channels;
    result = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "width", -1 ), Tcl_NewIntObj( out_w ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "height", -1 ), Tcl_NewIntObj( out_h ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "channel", -1 ), Tcl_NewIntObj( num_channels ));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj( "data", -1 ), Tcl_NewByteArrayObj( output_pixels, length));

    Tcl_SetObjResult(interp, result);
    if(output_pixels) ckfree(output_pixels);

    return TCL_OK;
}


static int tcl_stb_rgb2rgba(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
    int width = 0, height = 0;
    int x = 0, y = 0;
    unsigned char *input_pixels = NULL, *output_pixels = NULL;
    int len = 0;
    int length = 0;
    Tcl_Obj *result;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdata width height");
        return TCL_ERROR;
    }

    input_pixels = Tcl_GetByteArrayFromObj(objv[1], &len);
    if (!input_pixels || len < 1) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[3], &height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (len != width * height * 3) {
        return TCL_ERROR;
    }

    length = width * height * 4 * sizeof(unsigned char);
    output_pixels = (unsigned char *) ckalloc (length);
    if (!output_pixels) {
        return TCL_ERROR;
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width * 3; x += 3) {
            int location = y * width * 3 + x;

            *(output_pixels + location) = *(input_pixels + location);
            *(output_pixels + location + 1) = *(input_pixels + location + 1);
            *(output_pixels + location + 2) = *(input_pixels + location + 2);
            *(output_pixels + location + 3) = 255;
        }
    }

    result = Tcl_NewByteArrayObj( output_pixels, length);

    Tcl_SetObjResult(interp, result);
    if(output_pixels) ckfree(output_pixels);

    return TCL_OK;
}


static int tcl_stb_write(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
    char *format = NULL;
    char *filename = NULL;
    int len;
    int w = 0, h = 0, channels_in_file = 0;
    unsigned char *data = NULL;
    int result = 0;

    if (objc != 7) {
        Tcl_WrongNumArgs(interp, 1, objv, "format filename width height channels data");
        return TCL_ERROR;
    }

    format = Tcl_GetStringFromObj(objv[1], &len);
    if (!format || len < 1) {
        return TCL_ERROR;
    }

    filename = Tcl_GetStringFromObj(objv[2], &len);
    if (!filename || len < 1) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[3], &w) != TCL_OK) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[4], &h) != TCL_OK) {
        return TCL_ERROR;
    }

    if(Tcl_GetIntFromObj(interp, objv[5], &channels_in_file) != TCL_OK) {
        return TCL_ERROR;
    }

    data = Tcl_GetByteArrayFromObj(objv[6], &len);
    if (!data || len < 1) {
        return TCL_ERROR;
    }

    if (strcmp(format, "png")==0) {
        result = stbi_write_png(filename, w, h, channels_in_file, data, 0);
    } else if (strcmp(format, "jpg")==0) {
        result = stbi_write_jpg(filename, w, h, channels_in_file, data, 90);
    } else if (strcmp(format, "tga")==0) {
        result = stbi_write_tga(filename, w, h, channels_in_file, data);
    } else if (strcmp(format, "bmp")==0) {
        result = stbi_write_bmp(filename, w, h, channels_in_file, data);
    } else {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj( result ));

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Stbimage_Init --
 *
 *	Initialize the new package.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	The Stbimage_Init package is created.
 *
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
DLLEXPORT int
Stbimage_Init(Tcl_Interp *interp)
{
    Tcl_Namespace *nsPtr; /* pointer to hold our own new namespace */

    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    nsPtr = Tcl_CreateNamespace(interp, NS, NULL, NULL);
    if (nsPtr == NULL) {
        return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, NS "::load",
        (Tcl_ObjCmdProc *) tcl_stb_load,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, NS "::load_from_memory",
        (Tcl_ObjCmdProc *) tcl_stb_load_from_memory,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, NS "::resize",
        (Tcl_ObjCmdProc *) tcl_stb_resize,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, NS "::rgb2rgba",
        (Tcl_ObjCmdProc *) tcl_stb_rgb2rgba,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, NS "::write",
        (Tcl_ObjCmdProc *) tcl_stb_write,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
#ifdef __cplusplus
}
#endif  /* __cplusplus */
