#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STBI_MALLOC attemptckalloc
#define STBI_REALLOC attemptckrealloc
#define STBI_FREE ckfree
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STBIR_MALLOC(size, c) attemptckalloc(size)
#define STBIR_FREE(ptr, c) ckfree(ptr)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STBIW_MALLOC attemptckalloc
#define STBIW_REALLOC attemptckrealloc
#define STBIW_FREE ckfree
#define STBIW_WINDOWS_UTF8
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


static int tcl_stb_load(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    char *filename = NULL;
    int length = 0, len = 0, w = 0, h = 0, channels_in_file = 0;
    unsigned char *data = NULL;
    Tcl_Obj *result;
#ifndef _WIN32
    Tcl_DString ds;
#endif

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
    }

    filename = Tcl_GetStringFromObj(objv[1], &len);
    if (!filename || len < 1) {
        Tcl_SetResult(interp, "invalid filename", TCL_STATIC);
        return TCL_ERROR;
    }

#ifndef _WIN32
    filename = Tcl_UtfToExternalDString(NULL, filename, len, &ds);
#endif
    data = stbi_load(filename, &w, &h, &channels_in_file, 0);
#ifndef _WIN32
    Tcl_DStringFree(&ds);
#endif
    if (data == NULL) {
        Tcl_SetResult(interp, "load file failed", TCL_STATIC);
        return TCL_ERROR;
    }

    if (channels_in_file < 1 || channels_in_file > 4) {
        stbi_image_free(data);
        Tcl_SetResult(interp, "incompatible pixel format", TCL_STATIC);
        return TCL_ERROR;
    }


    length = w * h * channels_in_file;
    result = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("width", -1), Tcl_NewIntObj(w));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("height", -1), Tcl_NewIntObj(h));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("channels", -1), Tcl_NewIntObj(channels_in_file));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("data", -1), Tcl_NewByteArrayObj(data, length));

    Tcl_SetObjResult(interp, result);
    stbi_image_free(data);
    return TCL_OK;
}


static int tcl_stb_load_from_memory(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    unsigned char *filedata = NULL;
    int len, w = 0, h = 0, channels_in_file = 0, length = 0;
    unsigned char *data = NULL;
    Tcl_Obj *result;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "filedata");
        return TCL_ERROR;
    }

    filedata = Tcl_GetByteArrayFromObj(objv[1], &len);
    if (!filedata || len < 1) {
        Tcl_SetResult(interp, "invalid filedata", TCL_STATIC);
        return TCL_ERROR;
    }

    data = stbi_load_from_memory(filedata, len, &w, &h, &channels_in_file, 0);
    if (data == NULL) {
        Tcl_SetResult(interp, "load file failed", TCL_STATIC);
        return TCL_ERROR;
    }

    length = w * h * channels_in_file;
    result = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("width", -1), Tcl_NewIntObj(w));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("height", -1), Tcl_NewIntObj(h));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("channels", -1), Tcl_NewIntObj(channels_in_file));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("data", -1), Tcl_NewByteArrayObj(data, length));

    Tcl_SetObjResult(interp, result);
    stbi_image_free(data);
    return TCL_OK;
}


static int tcl_stb_resize(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
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
    if (input_pixels == NULL || len < 1) {
        Tcl_SetResult(interp, "invalid byte array", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in_w) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in_h) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[4], &out_w) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[5], &out_h) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[6], &num_channels) != TCL_OK) {
        return TCL_ERROR;
    }

    length = out_w * out_h * num_channels;
    output_pixels = (unsigned char *) attemptckalloc(length);
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }

    res = stbir_resize_uint8(input_pixels , in_w , in_h , 0,
                       output_pixels, out_w, out_h, 0,
                       num_channels);

    if (res == 0) {
        ckfree(output_pixels);
        Tcl_SetResult(interp, "resize failed", TCL_STATIC);
        return TCL_ERROR;
    }

    result = Tcl_NewDictObj();
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("width", -1), Tcl_NewIntObj(out_w));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("height", -1), Tcl_NewIntObj(out_h));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("channels", -1), Tcl_NewIntObj(num_channels));
    Tcl_DictObjPut(interp, result, Tcl_NewStringObj("data", -1), Tcl_NewByteArrayObj(output_pixels, length));

    Tcl_SetObjResult(interp, result);
    ckfree(output_pixels);

    return TCL_OK;
}


static int tcl_stb_rgb2rgba(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    int len = 0, length = 0, x = 0, y = 0, width = 0, height = 0;
    unsigned char *input_pixels = NULL, *output_pixels = NULL, *in_ptr, *out_ptr;
    Tcl_Obj *result;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "data width height");
        return TCL_ERROR;
    }

    input_pixels = Tcl_GetByteArrayFromObj(objv[1], &len);
    if (input_pixels == NULL || len < 1) {
        Tcl_SetResult(interp, "invalid byte array", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (len != width * height * 3) {
        Tcl_SetResult(interp, "invalid byte array length", TCL_STATIC);
        return TCL_ERROR;
    }

    length = width * height * 4 * sizeof(unsigned char);
    output_pixels = (unsigned char *) attemptckalloc(length);
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }

    out_ptr = output_pixels;
    in_ptr = input_pixels;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            *out_ptr++ = *in_ptr++;
            *out_ptr++ = *in_ptr++;
            *out_ptr++ = *in_ptr++;
            *out_ptr++ = 255;
        }
    }

    result = Tcl_NewByteArrayObj(output_pixels, length);

    Tcl_SetObjResult(interp, result);
    ckfree(output_pixels);

    return TCL_OK;
}


static int tcl_stb_write(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    char *format = NULL;
    char *filename = NULL;
    int len = 0, w = 0, h = 0, channels_in_file = 0, length = 0, result = 0;
    unsigned char *data = NULL;
#ifndef _WIN32
    Tcl_DString ds;
#endif

    if (objc != 7) {
        Tcl_WrongNumArgs(interp, 1, objv, "format filename width height channels data");
        return TCL_ERROR;
    }

    format = Tcl_GetStringFromObj(objv[1], &len);
    if (!format || len < 1) {
        Tcl_SetResult(interp, "invalid format", TCL_STATIC);
        return TCL_ERROR;
    }

    len = 0;
    filename = Tcl_GetStringFromObj(objv[2], &len);
    if (!filename || len < 1) {
        Tcl_SetResult(interp, "invalid filename", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &w) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &h) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[5], &channels_in_file) != TCL_OK) {
        return TCL_ERROR;
    }
    data = Tcl_GetByteArrayFromObj(objv[6], &length);
    if (data == NULL || length < 1) {
        Tcl_SetResult(interp, "invalid byte array", TCL_STATIC);
        return TCL_ERROR;
    }

#ifndef _WIN32
    filename = Tcl_UtfToExternalDString(NULL, filename, len, &ds);
#endif
    if (strcmp(format, "png") == 0) {
        result = stbi_write_png(filename, w, h, channels_in_file, data, 0);
    } else if (strcmp(format, "jpg") == 0) {
        result = stbi_write_jpg(filename, w, h, channels_in_file, data, 90);
    } else if (strcmp(format, "tga") == 0) {
        result = stbi_write_tga(filename, w, h, channels_in_file, data);
    } else if (strcmp(format, "bmp") == 0) {
        result = stbi_write_bmp(filename, w, h, channels_in_file, data);
    } else {
#ifndef _WIN32
        Tcl_DStringFree(&ds);
#endif
        Tcl_SetResult(interp, "unsupported output format", TCL_STATIC);
        return TCL_ERROR;
    }
#ifndef _WIN32
    Tcl_DStringFree(&ds);
#endif
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Stbimage_Init --
 *
 *      Initialize the new package.
 *
 * Results:
 *      A standard Tcl result
 *
 * Side effects:
 *      The "stbimage" package is created.
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

    Tcl_CreateObjCommand(interp, "::" NS "::load",
        (Tcl_ObjCmdProc *) tcl_stb_load,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::load_from_memory",
        (Tcl_ObjCmdProc *) tcl_stb_load_from_memory,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::resize",
        (Tcl_ObjCmdProc *) tcl_stb_resize,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::rgb2rgba",
        (Tcl_ObjCmdProc *) tcl_stb_rgb2rgba,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::write",
        (Tcl_ObjCmdProc *) tcl_stb_write,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
#ifdef __cplusplus
}
#endif  /* __cplusplus */

