/*
 * ffvarenderer_priv.h - VA renderer abstraction (private definitions)
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

#ifndef FFVA_RENDERER_PRIV_H
#define FFVA_RENDERER_PRIV_H

#include "ffvarenderer.h"

#define FFVA_RENDERER_CLASS(klass) \
    ((FFVARendererClass *)(klass))
#define FFVA_RENDERER_GET_CLASS(rnd) \
    FFVA_RENDERER_CLASS(FFVA_RENDERER(rnd)->klass)

typedef struct ffva_renderer_class_s    FFVARendererClass;

typedef bool (*FFVARendererInitFunc)(FFVARenderer *rnd, uint32_t flags);
typedef void (*FFVARendererFinalizeFunc)(FFVARenderer *rnd);
typedef bool (*FFVARendererGetSizeFunc)(FFVARenderer *rnd, uint32_t *width_ptr,
    uint32_t *height_ptr);
typedef bool (*FFVARendererSetSizeFunc)(FFVARenderer *rnd, uint32_t width,
    uint32_t height);
typedef bool (*FFVARendererPutSurfaceFunc)(FFVARenderer *rnd, FFVASurface *s,
    const VARectangle *src_rect, const VARectangle *dst_rect, uint32_t flags);

struct ffva_renderer_s {
    const void *klass;
    FFVARenderer *parent;
    FFVADisplay *display;
    void *window;
    uint32_t width;
    uint32_t height;
};

struct ffva_renderer_class_s {
    AVClass base;
    uint32_t size;
    FFVARendererType type;
    FFVARendererInitFunc init;
    FFVARendererFinalizeFunc finalize;
    FFVARendererGetSizeFunc get_size;
    FFVARendererSetSizeFunc set_size;
    FFVARendererPutSurfaceFunc put_surface;
};

DLL_HIDDEN
FFVARenderer *
ffva_renderer_new(const FFVARendererClass *klass, FFVADisplay *display,
    uint32_t flags);

#endif /* FFVA_RENDERER_PRIV_H */
