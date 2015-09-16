/*
 * ffvarenderer_drm.c - VA/DRM renderer
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

/* TODO: perform presentation with KMS APIs */

#include "sysdeps.h"
#include <va/va_drm.h>
#include "ffvarenderer_drm.h"
#include "ffvarenderer_priv.h"
#include "ffvadisplay_priv.h"

struct ffva_renderer_drm_s {
    FFVARenderer base;

    uint32_t display_width;
    uint32_t display_height;
};

static bool
renderer_init(FFVARendererDRM *rnd, uint32_t flags)
{
    FFVADisplay * const display = rnd->base.display;

    if (ffva_display_get_type(display) != FFVA_DISPLAY_TYPE_DRM)
        return false;
    return true;
}

static void
renderer_finalize(FFVARendererDRM *rnd)
{
}

static bool
renderer_get_size(FFVARendererDRM *rnd, uint32_t *width_ptr,
    uint32_t *height_ptr)
{
    if (width_ptr)
        *width_ptr = rnd->display_width;
    if (height_ptr)
        *height_ptr = rnd->display_height;
    return true;
}

static bool
renderer_set_size(FFVARendererDRM *rnd, uint32_t width, uint32_t height)
{
    rnd->display_width = width;
    rnd->display_height = height;
    return true;
}

static bool
renderer_put_surface(FFVARendererDRM *rnd, FFVASurface *surface,
    const VARectangle *src_rect, const VARectangle *dst_rect, uint32_t flags)
{
    return true;
}

static const FFVARendererClass *
ffva_renderer_drm_class(void)
{
    static const FFVARendererClass g_class = {
        .base = {
            .class_name = "FFVARendererDRM",
            .item_name  = av_default_item_name,
            .option     = NULL,
            .version    = LIBAVUTIL_VERSION_INT,
        },
        .size           = sizeof(FFVARendererDRM),
        .type           = FFVA_RENDERER_TYPE_DRM,
        .init           = (FFVARendererInitFunc)renderer_init,
        .finalize       = (FFVARendererFinalizeFunc)renderer_finalize,
        .get_size       = (FFVARendererGetSizeFunc)renderer_get_size,
        .set_size       = (FFVARendererSetSizeFunc)renderer_set_size,
        .put_surface    = (FFVARendererPutSurfaceFunc)renderer_put_surface,
    };
    return &g_class;
}

// Creates a new renderer object from the supplied VA display
FFVARenderer *
ffva_renderer_drm_new(FFVADisplay *display, uint32_t flags)
{
    return ffva_renderer_new(ffva_renderer_drm_class(), display, flags);
}
