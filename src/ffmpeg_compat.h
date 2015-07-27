/*
 * ffmpeg_compat.h - FFmpeg compatibility glue
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

#ifndef FFMPEG_COMPAT_H
#define FFMPEG_COMPAT_H

#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51,42,0)
enum AVPixelFormat {
    AV_PIX_FMT_NONE             = PIX_FMT_NONE,
    AV_PIX_FMT_GRAY8            = PIX_FMT_GRAY8,
    AV_PIX_FMT_GRAY16BE         = PIX_FMT_GRAY16BE,
    AV_PIX_FMT_GRAY16LE         = PIX_FMT_GRAY16LE,
    AV_PIX_FMT_YUV420P          = PIX_FMT_YUV420P,
    AV_PIX_FMT_YUV422P          = PIX_FMT_YUV422P,
    AV_PIX_FMT_YUV444P          = PIX_FMT_YUV444P,
    AV_PIX_FMT_YUV420P10BE      = PIX_FMT_YUV420P10BE,
    AV_PIX_FMT_YUV422P10BE      = PIX_FMT_YUV422P10BE,
    AV_PIX_FMT_YUV444P10BE      = PIX_FMT_YUV444P10BE,
    AV_PIX_FMT_YUV420P10LE      = PIX_FMT_YUV420P10LE,
    AV_PIX_FMT_YUV422P10LE      = PIX_FMT_YUV422P10LE,
    AV_PIX_FMT_YUV444P10LE      = PIX_FMT_YUV444P10LE,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,34,1)
    AV_PIX_FMT_YUV420P12BE      = PIX_FMT_YUV420P12BE,
    AV_PIX_FMT_YUV422P12BE      = PIX_FMT_YUV422P12BE,
    AV_PIX_FMT_YUV444P12BE      = PIX_FMT_YUV444P12BE,
    AV_PIX_FMT_YUV420P12LE      = PIX_FMT_YUV420P12LE,
    AV_PIX_FMT_YUV422P12LE      = PIX_FMT_YUV422P12LE,
    AV_PIX_FMT_YUV444P12LE      = PIX_FMT_YUV444P12LE,
#endif
    AV_PIX_FMT_YUV420P16BE      = PIX_FMT_YUV420P16BE,
    AV_PIX_FMT_YUV422P16BE      = PIX_FMT_YUV422P16BE,
    AV_PIX_FMT_YUV444P16BE      = PIX_FMT_YUV444P16BE,
    AV_PIX_FMT_YUV420P16LE      = PIX_FMT_YUV420P16LE,
    AV_PIX_FMT_YUV422P16LE      = PIX_FMT_YUV422P16LE,
    AV_PIX_FMT_YUV444P16LE      = PIX_FMT_YUV444P16LE,
    AV_PIX_FMT_NV12             = PIX_FMT_NV12,
    AV_PIX_FMT_YUYV422          = PIX_FMT_YUYV422,
    AV_PIX_FMT_UYVY422          = PIX_FMT_UYVY422,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,30,0)
    AV_PIX_FMT_0RGB             = PIX_FMT_0RGB,
    AV_PIX_FMT_RGB0             = PIX_FMT_RGB0,
    AV_PIX_FMT_0BGR             = PIX_FMT_0BGR,
    AV_PIX_FMT_BGR0             = PIX_FMT_BGR0,
#endif
    AV_PIX_FMT_VAAPI_VLD        = PIX_FMT_VAAPI_VLD,
};

#if AV_HAVE_BIGENDIAN
#define AV_PIX_FMT_NE(be, le) AV_PIX_FMT_##be
#else
#define AV_PIX_FMT_NE(be, le) AV_PIX_FMT_##le
#endif

#define AV_PIX_FMT_GRAY16 AV_PIX_FMT_NE(GRAY16BE, GRAY16LE)

#define AV_PIX_FMT_YUV420P10 AV_PIX_FMT_NE(YUV420P10BE, YUV420P10LE)
#define AV_PIX_FMT_YUV422P10 AV_PIX_FMT_NE(YUV422P10BE, YUV422P10LE)
#define AV_PIX_FMT_YUV444P10 AV_PIX_FMT_NE(YUV444P10BE, YUV444P10LE)
#if LIBAVUTIL_VERSION_INT > AV_VERSION_INT(51,34,0)
#define AV_PIX_FMT_YUV420P12 AV_PIX_FMT_NE(YUV420P12BE, YUV420P12LE)
#define AV_PIX_FMT_YUV422P12 AV_PIX_FMT_NE(YUV422P12BE, YUV422P12LE)
#define AV_PIX_FMT_YUV444P12 AV_PIX_FMT_NE(YUV444P12BE, YUV444P12LE)
#endif
#define AV_PIX_FMT_YUV420P16 AV_PIX_FMT_NE(YUV420P16BE, YUV420P16LE)
#define AV_PIX_FMT_YUV422P16 AV_PIX_FMT_NE(YUV422P16BE, YUV422P16LE)
#define AV_PIX_FMT_YUV444P16 AV_PIX_FMT_NE(YUV444P16BE, YUV444P16LE)
#endif

/* Checks whether library knows about the VP9 decoder */
#define FFMPEG_HAS_VP9_DECODER \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,0))

/* Checks whether library knows about the HEVC decoder */
#define FFMPEG_HAS_HEVC_DECODER \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,24,101))

/* Codec ids */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54,25,0)
enum AVCodecID {
    AV_CODEC_ID_RAWVIDEO        = CODEC_ID_RAWVIDEO,
    AV_CODEC_ID_MPEG1VIDEO      = CODEC_ID_MPEG1VIDEO,
    AV_CODEC_ID_MPEG2VIDEO      = CODEC_ID_MPEG2VIDEO,
    AV_CODEC_ID_MPEG4           = CODEC_ID_MPEG4,
    AV_CODEC_ID_MJPEG           = CODEC_ID_MJPEG,
    AV_CODEC_ID_H263            = CODEC_ID_H263,
    AV_CODEC_ID_H264            = CODEC_ID_H264,
    AV_CODEC_ID_WMV3            = CODEC_ID_WMV3,
    AV_CODEC_ID_VC1             = CODEC_ID_VC1,
    AV_CODEC_ID_VP8             = CODEC_ID_VP8,
#if FFMPEG_HAS_VP9_DECODER
    AV_CODEC_ID_VP9             = CODEC_ID_VP9,
#endif
#if FFMPEG_HAS_HEVC_DECODER
    AV_CODEC_ID_HEVC            = CODEC_ID_HEVC,
#endif
};
#endif

/* Profiles */
#ifndef FF_PROFILE_HEVC_MAIN
#define FF_PROFILE_HEVC_MAIN 1
#endif
#ifndef FF_PROFILE_HEVC_MAIN_10
#define FF_PROFILE_HEVC_MAIN_10 2
#endif
#ifndef FF_PROFILE_HEVC_MAIN_STILL_PICTURE
#define FF_PROFILE_HEVC_MAIN_STILL_PICTURE 3
#endif

/* AVFrame related utilities */
#define AV_FEATURE_AVFRAME_API \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,45,101))

#if !AV_FEATURE_AVFRAME_API
#define av_frame_alloc()        av_compat_frame_alloc()
#define av_frame_free(f)        av_compat_frame_free(f)

static inline AVFrame *
av_compat_frame_alloc(void)
{
    return avcodec_alloc_frame();
}

static inline void
av_compat_frame_free(AVFrame **frame_ptr)
{
    avcodec_free_frame(frame_ptr);
}
#endif

#endif /* FFMPEG_COMPAT_H */
