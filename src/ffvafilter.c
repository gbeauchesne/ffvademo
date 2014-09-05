/*
 * ffvafilter.c - FFmpeg/vaapi filter
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
#include "ffvafilter.h"
#include "ffvadisplay_priv.h"
#include "ffmpeg_utils.h"
#include "vaapi_utils.h"

#if USE_VA_VPP
# include <va/va_vpp.h>
#endif

struct ffva_filter_s {
    const void *klass;
    FFVADisplay *display;
    VADisplay va_display;
    VAConfigID va_config;
    VAContextID va_context;
    int pix_fmt;
    int *pix_fmts;
    VARectangle crop_rect;
    VARectangle target_rect;
    uint32_t use_crop_rect : 1;
    uint32_t use_target_rect : 1;
};

static bool
has_vpp(FFVADisplay *display)
{
    bool has_vpp = false;
#if USE_VA_VPP
    VAEntrypoint *va_entrypoints = NULL;
    int i, va_num_entrypoints;
    VAStatus va_status;

    if (!display)
        return false;

    va_num_entrypoints = vaMaxNumEntrypoints(display->va_display);
    va_entrypoints = malloc(va_num_entrypoints * sizeof(*va_entrypoints));
    if (!va_entrypoints)
        goto cleanup;

    va_status = vaQueryConfigEntrypoints(display->va_display, VAProfileNone,
        va_entrypoints, &va_num_entrypoints);
    if (!va_check_status(va_status, "vaQueryEntrypoints()"))
        goto cleanup;

    for (i = 0; !has_vpp && i < va_num_entrypoints; i++)
        has_vpp = va_entrypoints[i] == VAEntrypointVideoProc;

cleanup:
    free(va_entrypoints);
#endif
    return has_vpp;
}

static bool
ensure_formats(FFVAFilter *filter)
{
#if USE_VA_VPP
    VASurfaceAttrib *surface_attribs = NULL;
    uint32_t i, n, num_surface_attribs = 0;
    VAStatus va_status;

    if (filter->pix_fmts)
        return true;

    va_status = vaQuerySurfaceAttributes(filter->va_display, filter->va_config,
        NULL, &num_surface_attribs);
    if (!va_check_status(va_status, "vaQuerySurfaceAttributes()"))
        goto error;

    surface_attribs = malloc(num_surface_attribs * sizeof(*surface_attribs));
    if (!surface_attribs)
        goto error;

    va_status = vaQuerySurfaceAttributes(filter->va_display, filter->va_config,
        surface_attribs, &num_surface_attribs);
    if (!va_check_status(va_status, "vaQuerySurfaceAttributes()"))
        goto error;

    filter->pix_fmts = malloc((num_surface_attribs + 1) *
        sizeof(*filter->pix_fmts));
    if (!filter->pix_fmts)
        goto error;

    for (i = 0, n = 0; i < num_surface_attribs; i++) {
        const VASurfaceAttrib * const surface_attrib = &surface_attribs[i];
        enum AVPixelFormat pix_fmt;

        if (surface_attrib->type != VASurfaceAttribPixelFormat)
            continue;
        if (!(surface_attrib->flags & VA_SURFACE_ATTRIB_SETTABLE))
            continue;

        if (vaapi_to_ffmpeg_pix_fmt(surface_attrib->value.value.i, &pix_fmt))
            filter->pix_fmts[n++] = pix_fmt;
    }
    filter->pix_fmts[n] = AV_PIX_FMT_NONE;

    free(surface_attribs);
    return true;

error:
    free(surface_attribs);
#endif
    return false;
}

static bool
find_format(FFVAFilter *filter, int pix_fmt)
{
    int *p;

    if (filter->pix_fmts) {
        for (p = filter->pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
            if (*p == pix_fmt)
                return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------------- */
/* --- Interface                                                         --- */
/* ------------------------------------------------------------------------- */

static const AVClass *
ffva_filter_class(void)
{
    static const AVClass g_class = {
        .class_name     = "FFVAFilter",
        .item_name      = av_default_item_name,
        .option         = NULL,
        .version        = LIBAVUTIL_VERSION_INT,
    };
    return &g_class;
}

// Creates a new filter instance
FFVAFilter *
ffva_filter_new(FFVADisplay *display)
{
    FFVAFilter *filter;
    VAStatus va_status;

    if (!display || !has_vpp(display))
        return NULL;

    filter = calloc(1, sizeof(*filter));
    if (!filter)
        return NULL;

    filter->klass = ffva_filter_class();
    filter->display = display;
    filter->va_display = display->va_display;
    filter->va_config = VA_INVALID_ID;
    filter->va_context = VA_INVALID_ID;
    filter->pix_fmt = AV_PIX_FMT_NONE;

    va_status = vaCreateConfig(filter->va_display, VAProfileNone,
        VAEntrypointVideoProc, NULL, 0, &filter->va_config);
    if (!va_check_status(va_status, "vaCreateConfig()"))
        goto error;

    va_status = vaCreateContext(filter->va_display, filter->va_config, 0, 0, 0,
        NULL, 0, &filter->va_context);
    if (!va_check_status(va_status, "vaCreateContext()"))
        goto error;
    return filter;

error:
    ffva_filter_free(filter);
    return NULL;
}

// Destroys the supplied filter instance
void
ffva_filter_free(FFVAFilter *filter)
{
    if (!filter)
        return;

    if (filter->va_display) {
        va_destroy_context(filter->va_display, &filter->va_context);
        va_destroy_config(filter->va_display, &filter->va_config);
        filter->va_display = NULL;
    }
    av_freep(&filter->pix_fmts);
    free(filter);
}

// Releases filter instance and resets the supplied pointer to NULL
void
ffva_filter_freep(FFVAFilter **filter_ptr)
{
    if (!filter_ptr)
        return;
    ffva_filter_free(*filter_ptr);
    *filter_ptr = NULL;
}

// Applies the operations defined in filter to supplied surface
int
ffva_filter_process(FFVAFilter *filter, FFVASurface *src_surface,
    FFVASurface *dst_surface, uint32_t flags)
{
#if USE_VA_VPP
    VAProcPipelineParameterBuffer *va_vpp_params = NULL;
    VABufferID va_vpp_params_buf = VA_INVALID_ID;
    VAStatus va_status;
    const VARectangle *src_rect, *dst_rect;
    VARectangle src_rect_tmp, dst_rect_tmp;

    // Build surface region (source)
    if (filter->use_crop_rect) {
        src_rect = &filter->crop_rect;
        if (src_rect->x + src_rect->width > src_surface->width ||
            src_rect->y + src_rect->height > src_surface->height)
            return AVERROR(ERANGE);
    }
    else {
        src_rect = &src_rect_tmp;
        src_rect_tmp.x = 0;
        src_rect_tmp.y = 0;
        src_rect_tmp.width = src_surface->width;
        src_rect_tmp.height = src_surface->height;
    }

    // Build output region (target)
    if (filter->use_target_rect) {
        dst_rect = &filter->target_rect;
        if (dst_rect->x + dst_rect->width > dst_surface->width ||
            dst_rect->y + dst_rect->height > dst_surface->height)
            return AVERROR(ERANGE);
    }
    else {
        dst_rect = &dst_rect_tmp;
        dst_rect_tmp.x = 0;
        dst_rect_tmp.y = 0;
        dst_rect_tmp.width = dst_surface->width;
        dst_rect_tmp.height = dst_surface->height;
    }

    // Fill in VPP params
    if (!va_create_buffer(
            filter->va_display, filter->va_context,
            VAProcPipelineParameterBufferType, sizeof(*va_vpp_params), NULL,
            &va_vpp_params_buf, (void *)&va_vpp_params))
        goto error_create_buffer;

    memset(va_vpp_params, 0, sizeof(*va_vpp_params));
    va_vpp_params->surface = src_surface->id;
    va_vpp_params->surface_region = src_rect;
    va_vpp_params->surface_color_standard = VAProcColorStandardNone;
    va_vpp_params->output_region = dst_rect;
    va_vpp_params->output_color_standard = VAProcColorStandardNone;
    va_vpp_params->output_background_color = 0xff000000;
    va_vpp_params->filter_flags = flags;
    va_vpp_params->filters = NULL;
    va_vpp_params->num_filters = 0;

    va_unmap_buffer(filter->va_display, va_vpp_params_buf,
        (void **)&va_vpp_params);

    // Execute VPP pipeline
    va_status = vaBeginPicture(filter->va_display, filter->va_context,
        dst_surface->id);
    if (!va_check_status(va_status, "vaBeginPicture()"))
        goto error_vaapi_status;

    va_status = vaRenderPicture(filter->va_display, filter->va_context,
        &va_vpp_params_buf, 1);
    if (!va_check_status(va_status, "vaRenderPicture()"))
        goto error_vaapi_status;

    va_status = vaEndPicture(filter->va_display, filter->va_context);
    if (!va_check_status(va_status, "vaEndPicture()"))
        goto error_vaapi_status;

    va_destroy_buffer(filter->va_display, &va_vpp_params_buf);
    return 0;

error_create_buffer:
    return AVERROR(ENOMEM);
error_vaapi_status:
    va_destroy_buffer(filter->va_display, &va_vpp_params_buf);
    return vaapi_to_ffmpeg_error(va_status);
#endif
    return AVERROR(ENOSYS);
}

// Determines the set of supported target formats for video processing
const int *
ffva_filter_get_formats(FFVAFilter *filter)
{
    if (!filter || !ensure_formats(filter))
        return NULL;
    return filter->pix_fmts;
}

// Sets the desired pixel format of the resulting video processing operations
int
ffva_filter_set_format(FFVAFilter *filter, int pix_fmt)
{
    if (!filter)
        return AVERROR(EINVAL);

    if (!ensure_formats(filter))
        return AVERROR(ENOTSUP);

    if (pix_fmt != AV_PIX_FMT_NONE && !find_format(filter, pix_fmt))
        return AVERROR(ENOTSUP);

    filter->pix_fmt = pix_fmt;
    return AVERROR(ENOSYS);
}

// Sets the source surface cropping rectangle to use for video processing
int
ffva_filter_set_cropping_rectangle(FFVAFilter *filter, const VARectangle *rect)
{
    if (!filter)
        return AVERROR(EINVAL);

    filter->use_crop_rect = rect != NULL;
    if (filter->use_crop_rect)
        filter->crop_rect = *rect;
    return 0;
}

// Sets the region within the target surface where source surface is output
int
ffva_filter_set_target_rectangle(FFVAFilter *filter, const VARectangle *rect)
{
    if (!filter)
        return AVERROR(EINVAL);

    filter->use_target_rect = rect != NULL;
    if (filter->use_target_rect)
        filter->target_rect = *rect;
    return 0;
}
