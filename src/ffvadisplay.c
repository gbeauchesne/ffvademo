/*
 * ffvadisplay.c - VA display abstraction
 *
 * Copyright (C) 2014 Intel Corporation
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301
 */

#include "sysdeps.h"
#include "ffvadisplay.h"
#include "ffvadisplay_priv.h"
#include "vaapi_utils.h"

typedef bool (*FFVADisplayOpenFunc)(FFVADisplay *display);
typedef void (*FFVADisplayCloseFunc)(FFVADisplay *display);

/** Base display class */
typedef struct {
    AVClass base;
    uint32_t size;
    FFVADisplayType type;
    FFVADisplayOpenFunc open;
    FFVADisplayCloseFunc close;
} FFVADisplayClass;

/* ------------------------------------------------------------------------ */
/* --- X11 Display                                                      --- */
/* ------------------------------------------------------------------------ */

#if USE_VA_X11
#include <va/va_x11.h>

typedef struct {
    FFVADisplay base;
} FFVADisplayX11;

static bool
ffva_display_x11_open(FFVADisplayX11 *display)
{
    FFVADisplay * const base_display = (FFVADisplay*)display;

    base_display->native_display = XOpenDisplay(base_display->display_name);
    if (!base_display->native_display)
        goto error_open_display;

    base_display->va_display = vaGetDisplay(base_display->native_display);
    return true;

    /* ERRORS */
error_open_display:
    av_log(display, AV_LOG_ERROR, "failed to open display `%s'\n",
        base_display->display_name);
    return false;
}

static void
ffva_display_x11_close(FFVADisplayX11 *display)
{
    FFVADisplay * const base_display = (FFVADisplay*)display;

    if (base_display->native_display)
        XCloseDisplay(base_display->native_display);
}

static const FFVADisplayClass *
ffva_display_x11_class(void)
{
    static const FFVADisplayClass g_class = {
        .base = {
            .class_name = "FFVADisplayX11",
            .item_name  = av_default_item_name,
            .option     = NULL,
            .version    = LIBAVUTIL_VERSION_INT,
        },
        .size           = sizeof(FFVADisplayX11),
        .type           = FFVA_DISPLAY_TYPE_X11,
        .open           = (FFVADisplayOpenFunc)ffva_display_x11_open,
        .close          = (FFVADisplayCloseFunc)ffva_display_x11_close,
    };
    return &g_class;
}
#endif

/* ------------------------------------------------------------------------ */
/* --- Interface                                                        --- */
/* ------------------------------------------------------------------------ */

static inline const FFVADisplayClass *
ffva_display_class(void)
{
#if USE_VA_X11
    return ffva_display_x11_class();
#endif
    assert(0 && "unsupported VA backend");
    return NULL;
}

static bool
display_init(FFVADisplay *display, const char *name)
{
    const FFVADisplayClass * const klass = ffva_display_class();
    int major_version, minor_version;
    VAStatus va_status;

    if (name) {
        display->display_name = strdup(name);
        if (!display->display_name)
            return false;
    }

    display->klass = klass;

    if (klass->open && (!klass->open(display) || !display->va_display))
        return false;

    va_status = vaInitialize(display->va_display,
        &major_version, &minor_version);
    if (!va_check_status(va_status, "vaInitialize()"))
        return false;
    return true;
}

static void
display_finalize(FFVADisplay *display)
{
    const FFVADisplayClass * const klass = display->klass;

    if (display->va_display)
        vaTerminate(display->va_display);
    if (klass->close)
        klass->close(display);
    free(display->display_name);
}

// Creates a new display object and opens a connection to the native display
FFVADisplay *
ffva_display_new(const char *name)
{
    const FFVADisplayClass * const klass = ffva_display_class();
    FFVADisplay *display;

    display = calloc(1, klass->size);
    if (!display)
        return NULL;
    if (!display_init(display, name))
        goto error;
    return display;

error:
    ffva_display_free(display);
    return NULL;
}

// Closes the native display and deallocates all resources from FFVADisplay
void
ffva_display_free(FFVADisplay *display)
{
    if (!display)
        return;
    display_finalize(display);
    free(display);
}

// Releases FFVADisplay object and resets the supplied pointer to NULL
void
ffva_display_freep(FFVADisplay **display_ptr)
{
    if (display_ptr) {
        ffva_display_free(*display_ptr);
        *display_ptr = NULL;
    }
}

// Returns the type of the supplied display
FFVADisplayType
ffva_display_get_type(FFVADisplay *display)
{
    if (!display)
        return 0;
    return ((const FFVADisplayClass *)display->klass)->type;
}

// Returns the VA display
VADisplay
ffva_display_get_va_display(FFVADisplay *display)
{
    if (!display)
        return NULL;
    return display->va_display;
}

// Returns the native display
void *
ffva_display_get_native_display(FFVADisplay *display)
{
    if (!display)
        return NULL;
    return display->native_display;
}
