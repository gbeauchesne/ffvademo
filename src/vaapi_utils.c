/*
 * vaapi_utils.c - VA-API utilities
 *
 * Copyright (C) 2013 Intel Corporation
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
#include "vaapi_utils.h"

// Checks whether the VA status error needs to be printed out
static inline bool
va_check_status_is_quiet(VAStatus va_status)
{
    /* Only "unimplemented" status are quietly ignored */
    return va_status == VA_STATUS_ERROR_UNIMPLEMENTED;
}

// Checks the VA status
bool
va_check_status(VAStatus va_status, const char *msg)
{
    if (va_status != VA_STATUS_SUCCESS) {
        if (!va_check_status_is_quiet(va_status))
            fprintf(stderr, "error: %s: %s\n", msg, vaErrorStr(va_status));
        return false;
    }
    return true;
}

// Destroys a VA config
void
va_destroy_config(VADisplay dpy, VAConfigID *cfg_ptr)
{
    if (*cfg_ptr != VA_INVALID_ID) {
        vaDestroyConfig(dpy, *cfg_ptr);
        *cfg_ptr = VA_INVALID_ID;
    }
}

// Destroys a VA context
void
va_destroy_context(VADisplay dpy, VAContextID *ctx_ptr)
{
    if (*ctx_ptr != VA_INVALID_ID) {
        vaDestroyContext(dpy, *ctx_ptr);
        *ctx_ptr = VA_INVALID_ID;
    }
}

// Destroys a VA surface
void
va_destroy_surface(VADisplay dpy, VASurfaceID *surf_ptr)
{
    if (*surf_ptr != VA_INVALID_ID) {
        vaDestroySurfaces(dpy, surf_ptr, 1);
        *surf_ptr = VA_INVALID_ID;
    }
}

// Destroys a VA buffer
void
va_destroy_buffer(VADisplay dpy, VABufferID *buf_ptr)
{
    if (*buf_ptr != VA_INVALID_ID) {
        vaDestroyBuffer(dpy, *buf_ptr);
        *buf_ptr = VA_INVALID_ID;
    }
}

// Destroys an array of VA buffers
void
va_destroy_buffers(VADisplay dpy, VABufferID *buf, uint32_t *len_ptr)
{
    uint32_t i, num_buffers = *len_ptr;

    if (buf) {
        for (i = 0; i < num_buffers; i++)
            va_destroy_buffer(dpy, &buf[i]);
    }
    *len_ptr = 0;
}

// Creates and maps VA buffer
bool
va_create_buffer(VADisplay dpy, VAContextID ctx, int type, size_t size,
    const void *data, VABufferID *buf_id_ptr, void **mapped_data_ptr)
{
    VABufferID buf_id;
    VAStatus va_status;

    va_status = vaCreateBuffer(dpy, ctx, type, size, 1, (void *)data, &buf_id);
    if (!va_check_status(va_status, "vaCreateBuffer()"))
        return false;

    if (mapped_data_ptr) {
        data = va_map_buffer(dpy, buf_id);
        if (!data)
            goto error;
        *mapped_data_ptr = (void *)data;
    }

    *buf_id_ptr = buf_id;
    return true;

error:
    va_destroy_buffer(dpy, &buf_id);
    return false;
}

// Maps the specified VA buffer
void *
va_map_buffer(VADisplay dpy, VABufferID buf_id)
{
    VAStatus va_status;
    void *data = NULL;

    va_status = vaMapBuffer(dpy, buf_id, &data);
    if (!va_check_status(va_status, "vaMapBuffer()"))
        return NULL;
    return data;
}

// Unmaps the supplied VA buffer. Sets the (optional) data pointer to NULL
void
va_unmap_buffer(VADisplay dpy, VABufferID buf_id, void **buf_ptr)
{
    VAStatus va_status;

    if (buf_ptr)
        *buf_ptr = NULL;

    va_status = vaUnmapBuffer(dpy, buf_id);
    if (!va_check_status(va_status, "vaUnmapBuffer()"))
        return;
}

// Initializes image with safe default values
void
va_image_init_defaults(VAImage *image)
{
    if (!image)
        return;
    image->image_id = VA_INVALID_ID;
    image->buf = VA_INVALID_ID;
}
