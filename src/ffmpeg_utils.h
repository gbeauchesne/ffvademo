/*
 * ffmpeg_utils.h - FFmpeg utilities
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

#ifndef FFMPEG_UTILS_H
#define FFMPEG_UTILS_H

#include <va/va.h>
#include "ffmpeg_compat.h"

/** Returns a string representation of the supplied FFmpeg error id */
const char *
ffmpeg_strerror(int errnum, char errbuf[BUFSIZ]);

/** Translates FFmpeg codec and profile to VA profile */
bool
ffmpeg_to_vaapi_profile(enum AVCodecID ff_codec, int ff_profile,
    VAProfile *profile_ptr);

/** Translates FFmpeg pixel format to a VA fourcc */
bool
ffmpeg_to_vaapi_pix_fmt(enum AVPixelFormat pix_fmt, uint32_t *fourcc_ptr,
    uint32_t *chroma_ptr);

/** Translates VA fourcc to FFmpeg pixel format */
bool
vaapi_to_ffmpeg_pix_fmt(uint32_t fourcc, enum AVPixelFormat *pix_fmt_ptr);

/** Translates VA status to FFmpeg error */
int
vaapi_to_ffmpeg_error(VAStatus va_status);

#endif /* FFMPEG_UTILS_H */
