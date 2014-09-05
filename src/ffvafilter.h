/*
 * ffvafilter.h - FFmpeg/vaapi filter
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

#ifndef FFVA_FILTER_H
#define FFVA_FILTER_H

#include <stdint.h>
#include "ffvadisplay.h"
#include "ffvasurface.h"

typedef struct ffva_filter_s            FFVAFilter;

/** Creates a new filter instance */
FFVAFilter *
ffva_filter_new(FFVADisplay *display);

/** Destroys the supplied filter instance */
void
ffva_filter_free(FFVAFilter *filter);

/** Releases filter instance and resets the supplied pointer to NULL */
void
ffva_filter_freep(FFVAFilter **filter_ptr);

/** Applies the operations defined in filter to supplied surface */
int
ffva_filter_process(FFVAFilter *filter, FFVASurface *src_surface,
    FFVASurface *dst_surface, uint32_t flags);

/** Determines the set of supported target formats for video processing */
const int *
ffva_filter_get_formats(FFVAFilter *filter);

/** Sets the desired pixel format of the resulting video processing operations */
int
ffva_filter_set_format(FFVAFilter *filter, int pix_fmt);

/** Sets the source surface cropping rectangle to use for video processing */
int
ffva_filter_set_cropping_rectangle(FFVAFilter *filter, const VARectangle *rect);

/** Sets the region within the target surface where source surface is output */
int
ffva_filter_set_target_rectangle(FFVAFilter *filter, const VARectangle *rect);

#endif /* FFVA_FILTER_H */
