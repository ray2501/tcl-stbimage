#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STBI_MALLOC attemptckalloc
#define STBI_REALLOC attemptckrealloc
#define STBI_FREE ckfree
#ifdef _WIN32
#define STBI_WINDOWS_UTF8
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STBIR_MALLOC(size, c) attemptckalloc(size)
#define STBIR_FREE(ptr, c) ckfree(ptr)
#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define STBIW_MALLOC attemptckalloc
#define STBIW_REALLOC attemptckrealloc
#define STBIW_FREE ckfree
#ifdef _WIN32
#define STBIW_WINDOWS_UTF8
#endif
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

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

typedef struct {
    int tk_checked;
    Tcl_Obj *width_obj;
    Tcl_Obj *height_obj;
    Tcl_Obj *channels_obj;
    Tcl_Obj *data_obj;
    const Tcl_ObjType *dict_type;
} PkgData;


typedef struct {
    unsigned char *data;
    int length;
    int width;
    int height;
    int channels;
} ImgInfo;


static void set_result_dict(Tcl_Interp *interp, PkgData *pkg_data, unsigned char *pixels, int length, int width, int height, int channels) {
    Tcl_Obj *result = Tcl_NewDictObj();

    Tcl_DictObjPut(interp, result, pkg_data->width_obj, Tcl_NewIntObj(width));
    Tcl_DictObjPut(interp, result, pkg_data->height_obj, Tcl_NewIntObj(height));
    Tcl_DictObjPut(interp, result, pkg_data->channels_obj, Tcl_NewIntObj(channels));
    Tcl_DictObjPut(interp, result, pkg_data->data_obj, Tcl_NewByteArrayObj(pixels, length));

    Tcl_SetObjResult(interp, result);
}


static int get_img_info(Tcl_Interp *interp, PkgData *pkg_data, Tcl_Obj *obj, ImgInfo *info) {
    Tcl_Obj *elem;

    if (Tcl_DictObjGet(interp, obj, pkg_data->data_obj, &elem) != TCL_OK) {
        return TCL_ERROR;
    }
    info->data = Tcl_GetByteArrayFromObj(elem, &info->length);
    if (info->data == NULL || info->length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_DictObjGet(interp, obj, pkg_data->width_obj, &elem) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, elem, &info->width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_DictObjGet(interp, obj, pkg_data->height_obj, &elem) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, elem, &info->height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_DictObjGet(interp, obj, pkg_data->channels_obj, &elem) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, elem, &info->channels) != TCL_OK) {
        return TCL_ERROR;
    }

    if (info->length != info->channels * info->width * info->height) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }
    return TCL_OK;
}

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
    PkgData *pkg_data = (PkgData *) cd;
    int i, out_width = 0, out_height = 0;
    unsigned char *output_pixels = NULL;
    unsigned char *res = NULL;
    int length = 0;
    ImgInfo in;

    if (objc == 4) {
        if (get_img_info(interp, pkg_data, objv[1], &in) != TCL_OK) {
            return TCL_ERROR;
        }
        i = 2;
        goto get_dest;
    }

    if (objc != 7) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdict|inputdata ?srcwidth srcheight? dstwidth dstheight ?channels?");
        return TCL_ERROR;
    }

    in.data = Tcl_GetByteArrayFromObj(objv[1], &in.length);
    if (in.data == NULL || in.length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in.width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in.height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[6], &in.channels) != TCL_OK) {
        return TCL_ERROR;
    }
    i = 4;

get_dest:
    if (Tcl_GetIntFromObj(interp, objv[i], &out_width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[i+1], &out_height) != TCL_OK) {
        return TCL_ERROR;
    }

    length = in.width * in.height * in.channels;
    if (length <= 0 || length > in.length) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }

    length = out_width * out_height * in.channels;
    output_pixels = (unsigned char *) attemptckalloc(length);
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }

    res = stbir_resize_uint8_linear(in.data, in.width, in.height, 0,
                                    output_pixels, out_width, out_height, 0,
                                    (stbir_pixel_layout) in.channels);

    if (res == NULL) {
        ckfree(output_pixels);
        Tcl_SetResult(interp, "resize failed", TCL_STATIC);
        return TCL_ERROR;
    }

    set_result_dict(interp, pkg_data, output_pixels, length, out_width, out_height, in.channels);
    ckfree(output_pixels);

    return TCL_OK;
}


static int tcl_stb_rgb2rgba(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    int length = 0, x = 0, y = 0, grey = 0;
    unsigned char *output_pixels = NULL, *in_ptr, *out_ptr;
    ImgInfo in;
    Tcl_Obj *result;

    if (objc == 2) {
        if (get_img_info(interp, pkg_data, objv[1], &in) != TCL_OK) {
            return TCL_ERROR;
        }
        goto process;
    }

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdict|data ?width height?");
        return TCL_ERROR;
    }

    in.data = Tcl_GetByteArrayFromObj(objv[1], &in.length);
    if (in.data == NULL || in.length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in.width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in.height) != TCL_OK) {
        return TCL_ERROR;
    }

process:
    if (in.length == in.width * in.height) {
        grey = 1;
    } else if (in.length == in.width * in.height * 2) {
        grey = 2;
    } else if (in.length == in.width * in.height * 4) {
        /* identity */
    } else if (in.length != in.width * in.height * 3) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }

    length = in.width * in.height * 4;
    if (length == in.length) {
        output_pixels = in.data;
    } else {
        output_pixels = (unsigned char *) attemptckalloc(length);
    }
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }

    out_ptr = output_pixels;
    in_ptr = in.data;
    if (grey > 1) {
        for (y = 0; y < in.height; y++) {
            for (x = 0; x < in.width; x++) {
                *out_ptr++ = *in_ptr;
                *out_ptr++ = *in_ptr;
                *out_ptr++ = *in_ptr++;
                *out_ptr++ = *in_ptr++;
            }
        }
    } else if (grey) {
        for (y = 0; y < in.height; y++) {
            for (x = 0; x < in.width; x++) {
                *out_ptr++ = *in_ptr;
                *out_ptr++ = *in_ptr;
                *out_ptr++ = *in_ptr++;
                *out_ptr++ = 255;
            }
        }
    } else if (output_pixels != in.data) {
        for (y = 0; y < in.height; y++) {
            for (x = 0; x < in.width; x++) {
                *out_ptr++ = *in_ptr++;
                *out_ptr++ = *in_ptr++;
                *out_ptr++ = *in_ptr++;
                *out_ptr++ = 255;
            }
        }
    }

    if (objc == 2) {
        set_result_dict(interp, pkg_data, output_pixels, length, in.width, in.height, 4);
    } else {
        result = Tcl_NewByteArrayObj(output_pixels, length);
        Tcl_SetObjResult(interp, result);
    }

    if (output_pixels != in.data) {
        ckfree(output_pixels);
    }

    return TCL_OK;
}


static int tcl_stb_rgb2grey(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    int length = 0, x = 0, y = 0, grey = 0;
    unsigned char *output_pixels = NULL, *in_ptr, *out_ptr;
    ImgInfo in;
    Tcl_Obj *result;
    int with_alpha = 0;

    if (objc == 2) {
        if (get_img_info(interp, pkg_data, objv[1], &in) != TCL_OK) {
            return TCL_ERROR;
        }
        goto process;
    }

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdict|data ?width height?");
        return TCL_ERROR;
    }

    in.data = Tcl_GetByteArrayFromObj(objv[1], &in.length);
    if (in.data == NULL || in.length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in.width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in.height) != TCL_OK) {
        return TCL_ERROR;
    }
    with_alpha = 1;

process:
    if (in.length == in.width * in.height) {
        grey = 1;
    } else if (in.length == in.width * in.height * 2) {
        grey = 2;
        with_alpha = 1;
    } else if (in.length == in.width * in.height * 4) {
        /* RGBA */
        with_alpha = 2;
    } else if (in.length != in.width * in.height * 3) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }

    length = in.width * in.height * (with_alpha ? 2 : 1);
    if (length == in.length) {
        output_pixels = in.data;
    } else {
        output_pixels = (unsigned char *) attemptckalloc(length);
    }
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }

    out_ptr = output_pixels;
    in_ptr = in.data;
    if (grey > 1) {
        if (output_pixels != in.data) {
            for (y = 0; y < in.height; y++) {
                for (x = 0; x < in.width; x++) {
                    *out_ptr++ = *in_ptr;
                    *out_ptr++ = *in_ptr;
                }
            }
        }
    } else if (grey) {
        if (output_pixels != in.data) {
            for (y = 0; y < in.height; y++) {
                for (x = 0; x < in.width; x++) {
                    *out_ptr++ = *in_ptr;
                    if (with_alpha) {
                        *out_ptr++ = 255;
                    }
                }
            }
        }
    } else if (output_pixels != in.data) {
        for (y = 0; y < in.height; y++) {
            for (x = 0; x < in.width; x++) {
                int pixval;

                pixval  = 19518 * (*in_ptr++);
                pixval += 38319 * (*in_ptr++);
                pixval +=  7442 * (*in_ptr++);
                pixval = pixval >> 16;
                if (pixval > 255) {
                    pixval = 255;
                }
                *out_ptr++ = pixval;
                if (with_alpha > 1) {
                    *out_ptr++ = *in_ptr++;
                } else if (with_alpha) {
                    *out_ptr++ = 255;
                }
            }
        }
    }

    if (objc == 2) {
        set_result_dict(interp, pkg_data, output_pixels, length, in.width, in.height, with_alpha ? 2 : 1);
    } else {
        result = Tcl_NewByteArrayObj(output_pixels, length);
        Tcl_SetObjResult(interp, result);
    }

    if (output_pixels != in.data) {
        ckfree(output_pixels);
    }

    return TCL_OK;
}


static int tcl_stb_write(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    char *format = NULL, *filename, *filename2;
    int flen, result = 0;
    ImgInfo in;
    Tcl_DString ds;
#ifndef _WIN32
    Tcl_DString ds2;
#endif

    if (objc == 4 && objv[3]->typePtr == pkg_data->dict_type) {
        format = Tcl_GetStringFromObj(objv[1], &flen);
        if (flen < 1) {
            Tcl_SetResult(interp, "invalid format", TCL_STATIC);
            return TCL_ERROR;
        }

        filename = Tcl_GetStringFromObj(objv[2], &flen);
        if (flen < 1) {
            Tcl_SetResult(interp, "invalid filename", TCL_STATIC);
            return TCL_ERROR;
        }

        if (get_img_info(interp, pkg_data, objv[3], &in) != TCL_OK){
            return TCL_ERROR;
        }
        goto process;
    }

    if (objc != 4 && objc != 7) {
        Tcl_WrongNumArgs(interp, 1, objv, "format filename inputdict|width ?height channels data?");
        return TCL_ERROR;
    }

    format = Tcl_GetStringFromObj(objv[1], &flen);
    if (flen < 1) {
        Tcl_SetResult(interp, "invalid format", TCL_STATIC);
        return TCL_ERROR;
    }

    filename = Tcl_GetStringFromObj(objv[2], &flen);
    if (flen < 1) {
        Tcl_SetResult(interp, "invalid filename", TCL_STATIC);
        return TCL_ERROR;
    }

    if (objc == 7) {
        if (Tcl_GetIntFromObj(interp, objv[3], &in.width) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[4], &in.height) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[5], &in.channels) != TCL_OK) {
            return TCL_ERROR;
        }
        in.data = Tcl_GetByteArrayFromObj(objv[6], &in.length);
        if (in.data == NULL || in.length < in.width * in.height * in.channels) {
            Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
            return TCL_ERROR;
        }
    }

process:
    filename2 = Tcl_TranslateFileName(interp, filename, &ds);
    if (filename2 == NULL) {
        Tcl_ResetResult(interp);
        Tcl_DStringInit(&ds);
    } else {
        filename = filename2;
        flen = Tcl_DStringLength(&ds);
    }
#ifndef _WIN32
    filename = Tcl_UtfToExternalDString(NULL, filename, flen, &ds2);
#endif
    if (strcmp(format, "png") == 0) {
        result = stbi_write_png(filename, in.width, in.height, in.channels, in.data, 0);
    } else if (strcmp(format, "jpg") == 0) {
        result = stbi_write_jpg(filename, in.width, in.height, in.channels, in.data, 90);
    } else if (strcmp(format, "tga") == 0) {
        result = stbi_write_tga(filename, in.width, in.height, in.channels, in.data);
    } else if (strcmp(format, "bmp") == 0) {
        result = stbi_write_bmp(filename, in.width, in.height, in.channels, in.data);
    } else {
#ifndef _WIN32
        Tcl_DStringFree(&ds2);
#endif
        Tcl_DStringFree(&ds);
        Tcl_SetResult(interp, "unsupported output format", TCL_STATIC);
        return TCL_ERROR;
    }
#ifndef _WIN32
    Tcl_DStringFree(&ds2);
#endif
    Tcl_DStringFree(&ds);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));

    return TCL_OK;
}


static int ascii_art(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    int i, x, y, pixval, reverse = 0, out_width = 0, out_height = 0;
    unsigned char *output_pixels = NULL;
    unsigned char *res = NULL;
    int length = 0, indent_length = 0;
    const char *indent_string = NULL;
    ImgInfo in;
    Tcl_DString ds;

    if (objc == 4 || objc == 5) {
        if (get_img_info(interp, pkg_data, objv[1], &in) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc > 4) {
            indent_string = Tcl_GetStringFromObj(objv[4], &indent_length);
        }
        i = 2;
        goto get_dest;
    }

    if (objc < 7 || objc > 8) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdict|inputdata ?srcwidth srcheight? dstwidth dstheight ?channels? ?indent_string?");
        return TCL_ERROR;
    }

    in.data = Tcl_GetByteArrayFromObj(objv[1], &in.length);
    if (in.data == NULL || in.length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in.width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in.height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[6], &in.channels) != TCL_OK) {
        return TCL_ERROR;
    }
    if (in.channels < 0) {
        in.channels = -in.channels;
        reverse = 1;
    }

    if (objc > 7) {
        indent_string = Tcl_GetStringFromObj(objv[7], &indent_length);
    }
    i = 4;

get_dest:
    if (Tcl_GetIntFromObj(interp, objv[i], &out_width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[i+1], &out_height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (in.length <= 0 || in.length > in.width * in.height * in.channels) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }

    length = out_width * out_height * in.channels;
    output_pixels = (unsigned char *) attemptckalloc(length);
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }

    res = stbir_resize_uint8_linear(in.data, in.width, in.height, 0,
                                    output_pixels, out_width, out_height, 0,
                                    (stbir_pixel_layout) in.channels);

    if (res == NULL) {
        ckfree(output_pixels);
        Tcl_SetResult(interp, "resize failed", TCL_STATIC);
        return TCL_ERROR;
    }

    Tcl_DStringInit(&ds);
    for (y = 0; y < out_height; y++) {
        if (indent_string != NULL && indent_length) {
            Tcl_DStringAppend(&ds, indent_string, indent_length);
        }
        for (x = 0; x < out_width; x++) {
            char ch;

            if (in.channels == 1) {
                pixval = output_pixels[y * out_width + x];
            } else if (in.channels == 2) {
                pixval = output_pixels[(y * out_width + x) * 2];
            } else if (in.channels == 3) {
                pixval  = 19518 * output_pixels[(y * out_width + x) * 3 + 0];
                pixval += 38319 * output_pixels[(y * out_width + x) * 3 + 1];
                pixval +=  7442 * output_pixels[(y * out_width + x) * 3 + 2];
                pixval = pixval >> 16;
                if (pixval > 255) {
                    pixval = 255;
                }
            } else if (in.channels == 4) {
                pixval  = 19518 * output_pixels[(y * out_width + x) * 4 + 0];
                pixval += 38319 * output_pixels[(y * out_width + x) * 4 + 1];
                pixval +=  7442 * output_pixels[(y * out_width + x) * 4 + 2];
                pixval = pixval >> 16;
                if (pixval > 255) {
                    pixval = 255;
                }
            } else {
                pixval = 0;
            }
            if (reverse) {
                pixval = 255 - pixval;
            }
            if (pixval < 25) {
                ch = ' ';
            } else if (pixval < 50) {
                ch = '.';
            } else if (pixval < 75) {
                ch = '.';
            } else if (pixval < 100) {
                ch = ':';
            } else if (pixval < 125) {
                ch = '-';
            } else if (pixval < 150) {
                ch = '=';
            } else if (pixval < 175) {
                ch = '+';
            } else if (pixval < 200) {
                ch = '*';
            } else if (pixval < 225) {
                ch = '%';
            } else {
                ch = '@';
            }
            Tcl_DStringAppend(&ds, &ch, 1);
        }
        Tcl_DStringAppend(&ds, "\n", 1);
    }
    Tcl_DStringResult(interp, &ds);
    ckfree(output_pixels);

    return TCL_OK;
}


static int crop(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    int x = 0, y = 0, out_width = 0, out_height = 0, length = 0, i;
    unsigned char *output_pixels = NULL;
    ImgInfo in;

    if (objc == 6) {
        if (get_img_info(interp, pkg_data, objv[1], &in) != TCL_OK) {
            return TCL_ERROR;
        }
        i = 2;
        goto get_dest;
    }

    if (objc != 9) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdict|inputdata ?srcwidth srcheight? startcolumn startrow dstwidth dstheight ?channels?");
        return TCL_ERROR;
    }

    in.data = Tcl_GetByteArrayFromObj(objv[1], &in.length);
    if (in.data == NULL || in.length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in.width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in.height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[8], &in.channels) != TCL_OK) {
        return TCL_ERROR;
    }

    i = 4;

get_dest:
    if (Tcl_GetIntFromObj(interp, objv[i], &x) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[i+1], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[i+2], &out_width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[i+3], &out_height) != TCL_OK) {
        return TCL_ERROR;
    }

    length = in.width * in.height * in.channels;
    if (length <= 0 || length > in.length) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }

    if (x + out_width > in.width || y + out_height > in.height || x < 0 || y < 0 || out_width > in.width || out_height > in.height) {
        Tcl_SetResult(interp, "invalid crop region", TCL_STATIC);
        return TCL_ERROR;
    }

    length = out_width * out_height * in.channels;
    output_pixels = (unsigned char *) attemptckalloc(length);
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }

    for (i = 0; i < out_height; i++) {
        unsigned char *src, *dst;

        src = in.data + (y + i) * in.width * in.channels + x * in.channels;
        dst = output_pixels + i * out_width * in.channels;
        memcpy(dst, src, out_width * in.channels);
    }

    set_result_dict(interp, pkg_data, output_pixels, length, out_width, out_height, in.channels);
    ckfree(output_pixels);

    return TCL_OK;
}


static int mirror(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    int x, y, flipx, flipy, length, i;
    unsigned char *output_pixels = NULL;
    ImgInfo in;

    if (objc == 4) {
        if (get_img_info(interp, pkg_data, objv[1], &in) != TCL_OK) {
            return TCL_ERROR;
        }
        i = 2;
        goto get_flags;
    }

    if (objc != 7) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdict|inputdata ?width height channels? flipx flipy");
        return TCL_ERROR;
    }

    in.data = Tcl_GetByteArrayFromObj(objv[1], &in.length);
    if (in.data == NULL || in.length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in.width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in.height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[4], &in.channels) != TCL_OK) {
        return TCL_ERROR;
    }

    i = 5;

get_flags:
    if (Tcl_GetBooleanFromObj(interp, objv[i], &flipx) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetBooleanFromObj(interp, objv[i+1], &flipy) != TCL_OK) {
        return TCL_ERROR;
    }

    length = in.width * in.height * in.channels;
    if (length <= 0 || length > in.length) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }

    if (flipx || flipy) {
        output_pixels = (unsigned char *) attemptckalloc(length);
        if (output_pixels == NULL) {
            Tcl_SetResult(interp, "out of memory", TCL_STATIC);
            return TCL_ERROR;
        }
    } else {
        output_pixels = in.data;
    }

    if (flipy) {
        for (y = 0; y < in.height; y++) {
            unsigned char *src = in.data + y * in.width * in.channels;
            unsigned char *dst;

            if (flipx) {
                dst = output_pixels + (in.height - 1 - y) * in.width * in.channels;
            } else {
                dst = output_pixels + y * in.width * in.channels;
            }
            src += (in.width - 1) * in.channels;
            for (x = 0; x < in.width; x++) {
                if (in.channels > 1) {
                    memcpy(dst, src, in.channels);
                    dst += in.channels;
                    src -= in.channels;
                } else {
                    *dst++ = *src--;
                }
            }
        }
    } else if (flipx) {
        for (y = 0; y < in.height; y++) {
            memcpy(output_pixels + (in.height - 1 - y) * in.width * in.channels, in.data + y * in.width * in.channels, in.width * in.channels);
        }
    }

    set_result_dict(interp, pkg_data, output_pixels, length, in.width, in.height, in.channels);
    if (output_pixels != in.data) {
        ckfree(output_pixels);
    }

    return TCL_OK;
}


static int rotate(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    int x, y, angle, length, i;
    unsigned char *output_pixels = NULL;
    ImgInfo in;

    if (objc == 3) {
        if (get_img_info(interp, pkg_data, objv[1], &in) != TCL_OK) {
            return TCL_ERROR;
        }
        i = 2;
        goto get_angle;
    }

    if (objc != 6) {
        Tcl_WrongNumArgs(interp, 1, objv, "inputdict|inputdata ?width height channels? angle");
        return TCL_ERROR;
    }

    in.data = Tcl_GetByteArrayFromObj(objv[1], &in.length);
    if (in.data == NULL || in.length < 1) {
        Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &in.width) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[3], &in.height) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[4], &in.channels) != TCL_OK) {
        return TCL_ERROR;
    }

    i = 5;

get_angle:
    if (Tcl_GetIntFromObj(interp, objv[i], &angle) != TCL_OK) {
        return TCL_ERROR;
    }

    length = in.width * in.height * in.channels;
    if (length <= 0 || length > in.length) {
        Tcl_SetResult(interp, "invalid image size", TCL_STATIC);
        return TCL_ERROR;
    }

    angle = angle % 360;
    if (angle < 0) {
        angle = 360 + angle;
    }

    if (angle >= 45 && angle < 270 + 45) {
        output_pixels = (unsigned char *) attemptckalloc(length);
        if (output_pixels == NULL) {
            Tcl_SetResult(interp, "out of memory", TCL_STATIC);
            return TCL_ERROR;
        }
    } else {
        output_pixels = in.data;
    }

    if (angle >= 45 && angle < 90 + 45) {
        for (y = 0; y < in.height; y++) {
            unsigned char *src = in.data + y * in.width * in.channels;
            unsigned char *dst;

            for (x = 0; x < in.width; x++) {
                dst = output_pixels + (in.width - 1 - x) * in.height * in.channels;
                dst += y * in.channels;
                memcpy(dst, src, in.channels);
                src += in.channels;
            }
        }
        i = in.width;
        in.width = in.height;
        in.height = i;
    } else if (angle < 180 + 45) {
        for (y = 0; y < in.height; y++) {
            unsigned char *src = in.data + y * in.width * in.channels;
            unsigned char *dst = output_pixels + (in.height - 1 - y) * in.width * in.channels;

            src += (in.width - 1) * in.channels;
            for (x = 0; x < in.width; x++) {
                if (in.channels > 1) {
                    memcpy(dst, src, in.channels);
                    dst += in.channels;
                    src -= in.channels;
                } else {
                    *dst++ = *src--;
                }
            }
        }
    } else if (angle < 270 + 45) {
        for (y = 0; y < in.height; y++) {
            unsigned char *src = in.data + y * in.width * in.channels;
            unsigned char *dst;

            for (x = 0; x < in.width; x++) {
                dst = output_pixels + x * in.height * in.channels;
                dst += (in.height - 1 - y) * in.channels;
                memcpy(dst, src, in.channels);
                src += in.channels;
            }
        }
        i = in.width;
        in.width = in.height;
        in.height = i;
    }

    set_result_dict(interp, pkg_data, output_pixels, length, in.width, in.height, in.channels);
    if (output_pixels != in.data) {
        ckfree(output_pixels);
    }

    return TCL_OK;
}

/* see https://github.com/b-sullender/blend-pixels */

static inline void blend(unsigned char *dst, unsigned char *src, int rgb, int alpha) {
    double r1, g1, b1, a1, r2, g2, b2, a2;
    double alpha1, alpha2, rem, t1, t2, t3, t4;

    alpha1 = alpha / 255.0;
    r1 = dst[0] / 255.0;
    if (rgb) {
        b1 = dst[1] / 255.0;
        g1 = dst[2] / 255.0;
        a1 = dst[3] / 255.0;
    } else {
        a1 = dst[1] / 255.0;
    }
    r2 = src[0] / 255.0;
    if (rgb) {
        b2 = src[1] / 255.0;
        g2 = src[2] / 255.0;
        a2 = src[3] / 255.0;
    } else {
        a2 = src[1] / 255.0;
    }
    alpha2 = alpha1 / 1.0;
    alpha2 *= a2;
    rem = 1.0 - alpha2;
    t3 = rem;
    t4 = alpha2;
    rem *= a1;
    rem /= 1.0;
    alpha2 += (t3 - rem);
    t1 = r1 * rem;
    t2 = r2 * alpha2;
    r1 = round(((t1 + t2) / 1.0) * 255.0);
    if (r1 < 0.0) {
        dst[0] = 0;
    } else if (r1 > 255.0) {
        dst[0] = 255;
    } else {
        dst[0] = r1;
    }
    if (rgb) {
        t1 = b1 * rem;
        t2 = b2 * alpha2;
        b1 = round(((t1 + t2) / 1.0) * 255.0);
        if (b1 < 0.0) {
            dst[1] = 0;
        } else if (b1 > 255.0) {
            dst[1] = 255;
        } else {
            dst[1] = b1;
        }
        t1 = g1 * rem;
        t2 = g2 * alpha2;
        g1 = round(((t1 + t2) / 1.0) * 255.0);
        if (g1 < 0.0) {
            dst[2] = 0;
        } else if (g1 > 255.0) {
            dst[2] = 255;
        } else {
            dst[2] = g1;
        }
    }
    a1 = round(((a1 + t4 * (1.0 - a1) / 1.0)) * 255.0);
    if (a1 < 0.0) {
        alpha = 0;
    } else if (a1 > 255.0) {
        alpha = 255;
    } else {
        alpha = a1;
    }
    if (rgb) {
        dst[3] = alpha;
    } else {
        dst[1] = alpha;
    }
}


static int put(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj * const *objv) {
    PkgData *pkg_data = (PkgData *) cd;
    int x, y, xs, ys, pixval, alpha = 255, dst_x, dst_y, dst_w = 0, dst_h = 0, length, i, need;
    unsigned char *output_pixels = NULL, *src_ptr, *dst_ptr, src_buf[4], dst_buf[4];
    ImgInfo dst, src;

    if (objc < 5) {
wrong_args:
        Tcl_WrongNumArgs(interp, 1, objv, "put dstdict|dstdata ?dstwidth dstheight dstchannels? srcdict|srcdata ?srcwidth srcheight srcchannels? dstcolumn dstrow ?alpha width height?");
        return TCL_ERROR;
    }

    i = 1;
    need = 10;
    if (objv[1]->typePtr == pkg_data->dict_type) {
        if (get_img_info(interp, pkg_data, objv[1], &dst) != TCL_OK) {
            return TCL_ERROR;
        }
        need -= 4;
        i = 2;
    }

    if (need < 10 && objv[2]->typePtr == pkg_data->dict_type) {
        if (get_img_info(interp, pkg_data, objv[2], &src) != TCL_OK) {
            return TCL_ERROR;
        }
        need -= 4;
        i = 3;
    }

    if (objc < need) {
        goto wrong_args;
    }

    if (i == 1) {
        dst.data = Tcl_GetByteArrayFromObj(objv[1], &dst.length);
        if (dst.data == NULL || dst.length < 1) {
            Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
            return TCL_ERROR;
        }

        if (Tcl_GetIntFromObj(interp, objv[2], &dst.width) != TCL_OK) {
            return TCL_ERROR;
        }

        if (Tcl_GetIntFromObj(interp, objv[3], &dst.height) != TCL_OK) {
            return TCL_ERROR;
        }

        if (Tcl_GetIntFromObj(interp, objv[4], &dst.channels) != TCL_OK) {
            return TCL_ERROR;
        }

        length = dst.width * dst.height * dst.channels;
        if (length <= 0 || length > dst.length || dst.channels > 4) {
            Tcl_SetResult(interp, "invalid dst image size", TCL_STATIC);
            return TCL_ERROR;
        }

        i = 5;
    }

    if (i != 3) {
        src.data = Tcl_GetByteArrayFromObj(objv[i], &src.length);
        if (src.data == NULL || src.length < 1) {
            Tcl_SetResult(interp, "invalid bytearray", TCL_STATIC);
            return TCL_ERROR;
        }

        if (Tcl_GetIntFromObj(interp, objv[i+1], &src.width) != TCL_OK) {
            return TCL_ERROR;
        }

        if (Tcl_GetIntFromObj(interp, objv[i+2], &src.height) != TCL_OK) {
            return TCL_ERROR;
        }

        if (Tcl_GetIntFromObj(interp, objv[i+3], &src.channels) != TCL_OK) {
            return TCL_ERROR;
        }

        length = src.width * src.height * src.channels;
        if (length <= 0 || length > src.length || src.channels > 4) {
            Tcl_SetResult(interp, "invalid src image size", TCL_STATIC);
            return TCL_ERROR;
        }

        i += 4;
    }

    if (objc < i + 2) {
        goto wrong_args;
    }

    if (Tcl_GetIntFromObj(interp, objv[i], &dst_x) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[i+1], &dst_y) != TCL_OK) {
        return TCL_ERROR;
    }

    i += 2;

    if (objc >= i + 1) {
        if (Tcl_GetIntFromObj(interp, objv[i], &alpha) != TCL_OK) {
            return TCL_ERROR;
        }
        ++i;
    }

    if (objc >= i + 1) {
        if (Tcl_GetIntFromObj(interp, objv[i], &dst_w) != TCL_OK) {
            return TCL_ERROR;
        }
        ++i;
    }

    if (objc >= i + 1) {
        if (Tcl_GetIntFromObj(interp, objv[i], &dst_h) != TCL_OK) {
            return TCL_ERROR;
        }
        ++i;
    }

    if (objc > i) {
        goto wrong_args;
    }

    length = dst.width * dst.height * dst.channels;
    output_pixels = (unsigned char *) attemptckalloc(length);
    if (output_pixels == NULL) {
        Tcl_SetResult(interp, "out of memory", TCL_STATIC);
        return TCL_ERROR;
    }
    memcpy(output_pixels, dst.data, length);

    if (dst_w <= 0) {
        dst_w = src.width;
    }
    if (dst_w + dst_x > dst.width) {
        dst_w = dst.width - dst_x;
    }

    if (dst_h <= 0) {
        dst_h = src.height;
    }
    if (dst_h + dst_y > dst.height) {
        dst_h = dst.height - dst_y;
    }

    if (dst_w + dst_x < 0 || dst_x > dst.width ||
        dst_h + dst_y < 0 || dst_y > dst.height) {
        goto done;
    }

    y = dst_y;
    ys = 0;
    if (y < 0) {
        ys = -y;
        ys = ys % src.height;
        y = 0;
    }
    dst_ptr = output_pixels + y * dst.width * dst.channels;
    dst_w = dst_w * dst.channels;
    while (y < dst_h) {
        x = dst_x;
        xs = 0;
        if (x < 0) {
            xs = -x;
            xs = xs % src.width;
            x = 0;
        }
        x = x * dst.channels;
        while (x < dst_w) {
            src_ptr = src.data + (ys * src.width + xs) * src.channels;
            memcpy(src_buf, src_ptr, src.channels);
            if (src.channels == 1) {
                src_buf[1] = 255;
            } else if (src.channels == 3) {
                src_buf[3] = 255;
            }
            if (src.channels == 1 && dst.channels > 2) {
                src_buf[3] = 255;
                src_buf[1] = src_buf[0];
                src_buf[2] = src_buf[0];
            } else if (src.channels == 2 && dst.channels > 2) {
                src_buf[3] = src_buf[1];
                src_buf[1] = src_buf[0];
                src_buf[2] = src_buf[0];
            } else if (src.channels > 2 && dst.channels < 3) {
                pixval  = 19518 * src_buf[0];
                pixval += 38319 * src_buf[1];
                pixval +=  7442 * src_buf[2];
                pixval = pixval >> 16;
                if (pixval > 255) {
                    pixval = 255;
                }
                src_buf[0] = pixval;
                if (src.channels < 4) {
                    src_buf[1] = 255;
                } else {
                    src_buf[1] = src_buf[3];
                }
            }
            memcpy(dst_buf, dst_ptr + x, dst.channels);
            if (dst.channels == 1) {
                dst_buf[1] = 255;
            } else if (dst.channels == 3) {
                dst_buf[3] = 255;
            }
            blend(dst_buf, src_buf, dst.channels > 2, alpha);
            memcpy(dst_ptr + x, dst_buf, dst.channels);
            if (++xs >= src.width) {
                xs = 0;
            }
            x += dst.channels;
        }
        if (++ys >= src.height) {
            ys = 0;
        }
        dst_ptr += dst.width * dst.channels;
        ++y;
    }

done:
    set_result_dict(interp, pkg_data, output_pixels, length, dst.width, dst.height, dst.channels);
    ckfree(output_pixels);

    return TCL_OK;
}


static void pkg_cleanup(void *cd, Tcl_Interp *interp) {
    PkgData *pkg_data = (PkgData *) cd;

    Tcl_DecrRefCount(pkg_data->width_obj);
    Tcl_DecrRefCount(pkg_data->height_obj);
    Tcl_DecrRefCount(pkg_data->channels_obj);
    Tcl_DecrRefCount(pkg_data->data_obj);
    ckfree(pkg_data);
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
    PkgData *pkg_data;

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

    pkg_data = (PkgData *) ckalloc(sizeof(PkgData));
    pkg_data->tk_checked = 0;

    pkg_data->width_obj = Tcl_NewStringObj("width", -1);
    pkg_data->height_obj = Tcl_NewStringObj("height", -1);
    pkg_data->channels_obj = Tcl_NewStringObj("channels", -1);
    pkg_data->data_obj = Tcl_NewStringObj("data", -1);
    pkg_data->dict_type = Tcl_GetObjType("dict");

    Tcl_IncrRefCount(pkg_data->width_obj);
    Tcl_IncrRefCount(pkg_data->height_obj);
    Tcl_IncrRefCount(pkg_data->channels_obj);
    Tcl_IncrRefCount(pkg_data->data_obj);


    Tcl_CreateObjCommand(interp, "::" NS "::load",
        (Tcl_ObjCmdProc *) tcl_stb_load,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::load_from_memory",
        (Tcl_ObjCmdProc *) tcl_stb_load_from_memory,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::resize",
        (Tcl_ObjCmdProc *) tcl_stb_resize,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::rgb2rgba",
        (Tcl_ObjCmdProc *) tcl_stb_rgb2rgba,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::rgb2grey",
        (Tcl_ObjCmdProc *) tcl_stb_rgb2grey,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::write",
        (Tcl_ObjCmdProc *) tcl_stb_write,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::ascii_art",
        (Tcl_ObjCmdProc *) ascii_art,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::crop",
        (Tcl_ObjCmdProc *) crop,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::mirror",
        (Tcl_ObjCmdProc *) mirror,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::rotate",
        (Tcl_ObjCmdProc *) rotate,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::put",
        (Tcl_ObjCmdProc *) put,
        (ClientData) pkg_data, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CallWhenDeleted(interp, pkg_cleanup, (ClientData) pkg_data);

    return TCL_OK;
}
#ifdef __cplusplus
}
#endif  /* __cplusplus */

