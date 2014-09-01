/*
 * ffvadisplay.h - VA display abstraction
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

#ifndef FFVA_DISPLAY_H
#define FFVA_DISPLAY_H

#include <va/va.h>

typedef struct ffva_display_s           FFVADisplay;

typedef enum {
    FFVA_DISPLAY_TYPE_X11 = 1,
} FFVADisplayType;

/** Creates a new display object and opens a connection to the native display */
FFVADisplay *
ffva_display_new(const char *name);

/** Closes the native display and deallocates all resources from FFVADisplay */
void
ffva_display_free(FFVADisplay *display);

/** Releases FFVADisplay object and resets the supplied pointer to NULL */
void
ffva_display_freep(FFVADisplay **display_ptr);

/** Returns the type of the supplied display */
FFVADisplayType
ffva_display_get_type(FFVADisplay *display);

/** Returns the VA display */
VADisplay
ffva_display_get_va_display(FFVADisplay *display);

/** Returns the native display */
void *
ffva_display_get_native_display(FFVADisplay *display);

#endif /* FFVA_DISPLAY_H */
