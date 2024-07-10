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
#include "stb_image_resize2.h"
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
    int in_w = 0, in_h = 0, out_w = 0, out_h = 0, num_channels = 0;
    unsigned char *input_pixels = NULL, *output_pixels = NULL;
    int len = 0;
    int length = 0;
    unsigned char *res = NULL;
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

    res = stbir_resize_uint8_linear(input_pixels , in_w , in_h , 0,
                       output_pixels, out_w, out_h, 0,
                       (stbir_pixel_layout) num_channels);

    if (res == NULL) {
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
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::rgb2rgba",
        (Tcl_ObjCmdProc *) tcl_stb_rgb2rgba,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand(interp, "::" NS "::write",
        (Tcl_ObjCmdProc *) tcl_stb_write,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

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

    Tcl_CallWhenDeleted(interp, pkg_cleanup, (ClientData) pkg_data);

    return TCL_OK;
}
#ifdef __cplusplus
}
#endif  /* __cplusplus */

