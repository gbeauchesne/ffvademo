/*
 * ffmpeg_utils.c - FFmpeg utilities
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
#include "ffmpeg_utils.h"
#include "ffmpeg_compat.h"
#include "vaapi_compat.h"

// Returns a string representation of the supplied FFmpeg error id
const char *
ffmpeg_strerror(int errnum, char errbuf[BUFSIZ])
{
    if (av_strerror(errnum, errbuf, BUFSIZ) != 0)
        sprintf(errbuf, "error %d", errnum);
    return errbuf;
}

// Translates FFmpeg codec and profile to VA profile
bool
ffmpeg_to_vaapi_profile(enum AVCodecID ff_codec, int ff_profile,
    VAProfile *profile_ptr)
{
    int profile = -1;

#define DEFINE_PROFILE(CODEC, FFMPEG_PROFILE, VA_PROFILE)       \
    case U_GEN_CONCAT4(FF_PROFILE_,CODEC,_,FFMPEG_PROFILE):     \
        profile = U_GEN_CONCAT3(VAProfile,CODEC,VA_PROFILE);    \
        break

    switch (ff_codec) {
    case AV_CODEC_ID_MPEG2VIDEO:
        switch (ff_profile) {
            DEFINE_PROFILE(MPEG2, SIMPLE, Simple);
            DEFINE_PROFILE(MPEG2, MAIN, Main);
        }
        break;
    case AV_CODEC_ID_MPEG4:
        switch (ff_profile) {
            DEFINE_PROFILE(MPEG4, SIMPLE, Simple);
            DEFINE_PROFILE(MPEG4, MAIN, Main);
            DEFINE_PROFILE(MPEG4, ADVANCED_SIMPLE, AdvancedSimple);
        }
        break;
    case AV_CODEC_ID_H264:
        switch (ff_profile) {
            DEFINE_PROFILE(H264, BASELINE, Baseline);
            DEFINE_PROFILE(H264, CONSTRAINED_BASELINE, ConstrainedBaseline);
            DEFINE_PROFILE(H264, MAIN, Main);
            DEFINE_PROFILE(H264, HIGH, High);
        }
        break;
    case AV_CODEC_ID_VC1:
        switch (ff_profile) {
            DEFINE_PROFILE(VC1, SIMPLE, Simple);
            DEFINE_PROFILE(VC1, MAIN, Main);
            DEFINE_PROFILE(VC1, ADVANCED, Advanced);
        }
        break;
#if FFMPEG_HAS_HEVC_DECODER
    case AV_CODEC_ID_HEVC:
        switch (ff_profile) {
            DEFINE_PROFILE(HEVC, MAIN, Main);
            DEFINE_PROFILE(HEVC, MAIN_10, Main10);
        }
        break;
#endif
    case AV_CODEC_ID_VP8:
        profile = VAProfileVP8Version0_3;
        break;
#if FFMPEG_HAS_VP9_DECODER
    case AV_CODEC_ID_VP9:
        profile = VAProfileVP9Version0;
        break;
#endif
    default:
        break;
    }
#undef DEFINE_PROFILE

    if (profile_ptr)
        *profile_ptr = profile;
    return profile != -1;
}

struct ffva_pix_fmt_map {
    enum AVPixelFormat pix_fmt;
    uint32_t va_fourcc;
    uint32_t va_chroma;
};

#define DEFINE_FORMAT(FF_FORMAT, FOURCC, CHROMA)                \
    { U_GEN_CONCAT(AV_PIX_FMT_,FF_FORMAT), VA_FOURCC FOURCC,    \
            U_GEN_CONCAT(VA_RT_FORMAT_,CHROMA) }

static const struct ffva_pix_fmt_map g_ffva_pix_fmt_map[] = {
    DEFINE_FORMAT(GRAY8,                ('Y','8','0','0'), YUV400),
    DEFINE_FORMAT(YUV420P,              ('I','4','2','0'), YUV420),
    DEFINE_FORMAT(NV12,                 ('N','V','1','2'), YUV420),
    DEFINE_FORMAT(YUYV422,              ('Y','U','Y','2'), YUV422),
    DEFINE_FORMAT(UYVY422,              ('U','Y','V','Y'), YUV422),
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,30,0)
    DEFINE_FORMAT(NE(RGB0, 0BGR),       ('R','G','B','X'), RGB32),
    DEFINE_FORMAT(NE(BGR0, 0RGB),       ('B','G','R','X'), RGB32),
    DEFINE_FORMAT(NE(RGBA, ABGR),       ('R','G','B','A'), RGB32),
    DEFINE_FORMAT(NE(BGRA, ARGB),       ('B','G','R','A'), RGB32),
#endif
    { AV_PIX_FMT_NONE, }
};
#undef DEFINE_FORMAT

// Translates FFmpeg pixel format to a VA fourcc
bool
ffmpeg_to_vaapi_pix_fmt(enum AVPixelFormat pix_fmt, uint32_t *fourcc_ptr,
    uint32_t *chroma_ptr)
{
    const struct ffva_pix_fmt_map *m;

    for (m = g_ffva_pix_fmt_map; m->pix_fmt != AV_PIX_FMT_NONE; m++) {
        if (m->pix_fmt == pix_fmt)
            break;
    }

    if (fourcc_ptr)
        *fourcc_ptr = m->va_fourcc;
    if (chroma_ptr)
        *chroma_ptr = m->va_chroma;
    return m->pix_fmt != AV_PIX_FMT_NONE;
}

// Translates VA fourcc to FFmpeg pixel format
bool
vaapi_to_ffmpeg_pix_fmt(uint32_t fourcc, enum AVPixelFormat *pix_fmt_ptr)
{
    const struct ffva_pix_fmt_map *m;

    for (m = g_ffva_pix_fmt_map; m->va_fourcc != 0; m++) {
        if (m->va_fourcc == fourcc)
            break;
    }

    if (pix_fmt_ptr)
        *pix_fmt_ptr = m->pix_fmt;
    return m->pix_fmt != AV_PIX_FMT_NONE;
}

// Translates VA status to FFmpeg error
int
vaapi_to_ffmpeg_error(VAStatus va_status)
{
    int ret;

    switch (va_status) {
    case VA_STATUS_ERROR_OPERATION_FAILED:
        ret = AVERROR(ENOTSUP);
        break;
    case VA_STATUS_ERROR_INVALID_DISPLAY:
    case VA_STATUS_ERROR_INVALID_CONFIG:
    case VA_STATUS_ERROR_INVALID_CONTEXT:
    case VA_STATUS_ERROR_INVALID_SURFACE:
    case VA_STATUS_ERROR_INVALID_BUFFER:
    case VA_STATUS_ERROR_INVALID_IMAGE:
    case VA_STATUS_ERROR_INVALID_SUBPICTURE:
        ret = AVERROR(EINVAL);
        break;
    case VA_STATUS_ERROR_INVALID_PARAMETER:
    case VA_STATUS_ERROR_INVALID_VALUE:
        ret = AVERROR(EINVAL);
        break;
    case VA_STATUS_ERROR_ALLOCATION_FAILED:
        ret = AVERROR(ENOMEM);
        break;
    case VA_STATUS_ERROR_UNIMPLEMENTED:
        ret = AVERROR(ENOSYS);
        break;
    case VA_STATUS_ERROR_SURFACE_BUSY:
        ret = AVERROR(EBUSY);
        break;
    default:
        ret = AVERROR_UNKNOWN;
        break;
    }
    return ret;
}
