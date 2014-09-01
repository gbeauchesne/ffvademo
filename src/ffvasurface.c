/*
 * ffvasurface.c - VA surface abstraction
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
#include "ffvasurface.h"

// Initializes VA surface holder with sane defaults
void
ffva_surface_init_defaults(FFVASurface *s)
{
    if (!s)
        return;

    s->id = VA_INVALID_ID;
    s->chroma = 0;
    s->fourcc = 0;
    s->width = 0;
    s->height = 0;
}

// Initializes VA surface holder with the supplied info
void
ffva_surface_init(FFVASurface *s, VASurfaceID id, uint32_t chroma,
    uint32_t width, uint32_t height)
{
    if (!s)
        return;

    s->id = id;
    s->chroma = chroma;
    s->fourcc = 0;
    s->width = width;
    s->height = height;
}
