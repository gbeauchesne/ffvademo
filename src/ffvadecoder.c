/*
 * ffvadecoder.c - FFmpeg/vaapi decoder
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
#include <pthread.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavcodec/vaapi.h>
#include "ffvadecoder.h"
#include "ffvadisplay.h"
#include "ffvadisplay_priv.h"
#include "ffvasurface.h"
#include "ffmpeg_compat.h"
#include "ffmpeg_utils.h"
#include "vaapi_utils.h"

enum {
    STATE_INITIALIZED   = 1 << 0,
    STATE_OPENED        = 1 << 1,
    STATE_STARTED       = 1 << 2,
};

struct ffva_decoder_s {
    const void *klass;
    AVFormatContext *fmtctx;
    AVStream *stream;
    AVCodecContext *avctx;
    AVFrame *frame;

    FFVADisplay *display;
    struct vaapi_context va_context;
    VAProfile *va_profiles;
    uint32_t num_va_profiles;
    FFVASurface *va_surfaces;
    uint32_t num_va_surfaces;
    FFVASurface **va_surfaces_queue;
    uint32_t va_surfaces_queue_length;
    uint32_t va_surfaces_queue_head;
    uint32_t va_surfaces_queue_tail;

    volatile uint32_t state;
    FFVADecoderFrame decoded_frame;
};

/* ------------------------------------------------------------------------ */
/* --- VA-API Decoder                                                   --- */
/* ------------------------------------------------------------------------ */

// Ensures the array of VA profiles is allocated and filled up correctly
static int
vaapi_ensure_profiles(FFVADecoder *dec)
{
    struct vaapi_context * const vactx = &dec->va_context;
    VAProfile *profiles;
    int num_profiles;
    VAStatus va_status;

    if (dec->va_profiles && dec->num_va_profiles > 0)
        return 0;

    num_profiles = vaMaxNumProfiles(vactx->display);
    profiles = malloc(num_profiles * sizeof(*profiles));
    if (!profiles)
        return AVERROR(ENOMEM);

    va_status = vaQueryConfigProfiles(vactx->display, profiles, &num_profiles);
    if (!va_check_status(va_status, "vaQueryConfigProfiles()"))
        goto error_query_profiles;

    dec->va_profiles = profiles;
    dec->num_va_profiles = num_profiles;
    return 0;

    /* ERRORS */
error_query_profiles:
    av_log(dec, AV_LOG_ERROR, "failed to query the set of supported profiles\n");
    free(profiles);
    return vaapi_to_ffmpeg_error(va_status);
}

// Ensures the array of VA surfaces and queue of free VA surfaces are allocated
static int
vaapi_ensure_surfaces(FFVADecoder *dec, uint32_t num_surfaces)
{
    uint32_t i, size, new_size;
    void *mem;

    size = dec->num_va_surfaces * sizeof(*dec->va_surfaces);
    new_size = num_surfaces * sizeof(*dec->va_surfaces);
    mem = av_fast_realloc(dec->va_surfaces, &size, new_size);
    if (!mem)
        goto error_alloc_surfaces;
    dec->va_surfaces = mem;

    if (dec->num_va_surfaces < num_surfaces) {
        for (i = dec->num_va_surfaces; i < num_surfaces; i++)
            ffva_surface_init_defaults(&dec->va_surfaces[i]);
        dec->num_va_surfaces = num_surfaces;
    }

    size = dec->va_surfaces_queue_length * sizeof(*dec->va_surfaces_queue);
    new_size = num_surfaces * sizeof(*dec->va_surfaces_queue);
    mem = av_fast_realloc(dec->va_surfaces_queue, &size, new_size);
    if (!mem)
        goto error_alloc_surfaces_queue;
    dec->va_surfaces_queue = mem;

    if (dec->va_surfaces_queue_length < num_surfaces) {
        for (i = dec->va_surfaces_queue_length; i < num_surfaces; i++)
            dec->va_surfaces_queue[i] = NULL;
        dec->va_surfaces_queue_length = num_surfaces;
    }
    return 0;

    /* ERRORS */
error_alloc_surfaces:
    av_log(dec, AV_LOG_ERROR, "failed to allocate VA surfaces array\n");
    return AVERROR(ENOMEM);
error_alloc_surfaces_queue:
    av_log(dec, AV_LOG_ERROR, "failed to allocate VA surfaces queue\n");
    return AVERROR(ENOMEM);
}

// Acquires a surface from the queue of free VA surfaces
static int
vaapi_acquire_surface(FFVADecoder *dec, FFVASurface **out_surface_ptr)
{
    FFVASurface *surface;

    surface = dec->va_surfaces_queue[dec->va_surfaces_queue_head];
    if (!surface)
        return AVERROR_BUG;

    dec->va_surfaces_queue[dec->va_surfaces_queue_head] = NULL;
    dec->va_surfaces_queue_head = (dec->va_surfaces_queue_head + 1) %
        dec->va_surfaces_queue_length;

    if (out_surface_ptr)
        *out_surface_ptr = surface;
    return 0;
}

// Releases a surface back to the queue of free VA surfaces
static int
vaapi_release_surface(FFVADecoder *dec, FFVASurface *s)
{
    FFVASurface * const surface_in_queue =
        dec->va_surfaces_queue[dec->va_surfaces_queue_tail];

    if (surface_in_queue)
        return AVERROR_BUG;

    dec->va_surfaces_queue[dec->va_surfaces_queue_tail] = s;
    dec->va_surfaces_queue_tail = (dec->va_surfaces_queue_tail + 1) %
        dec->va_surfaces_queue_length;
    return 0;
}

// Checks whether the supplied config, i.e. (profile, entrypoint) pair, exists
static bool
vaapi_has_config(FFVADecoder *dec, VAProfile profile, VAEntrypoint entrypoint)
{
    uint32_t i;

    if (vaapi_ensure_profiles(dec) != 0)
        return false;

    for (i = 0; i < dec->num_va_profiles; i++) {
        if (dec->va_profiles[i] == profile)
            break;
    }
    if (i == dec->num_va_profiles)
        return false;
    return true;
}

// Initializes VA decoder comprising of VA config, surfaces and context
static int
vaapi_init_decoder(FFVADecoder *dec, VAProfile profile, VAEntrypoint entrypoint)
{
    AVCodecContext * const avctx = dec->avctx;
    struct vaapi_context * const vactx = &dec->va_context;
    VAConfigID va_config = VA_INVALID_ID;
    VAContextID va_context = VA_INVALID_ID;
    VAConfigAttrib va_attribs[1], *va_attrib;
    uint32_t i, num_va_attribs = 0;
    VASurfaceID *va_surfaces = NULL;
    VAStatus va_status;
    int ret;

    va_attrib = &va_attribs[num_va_attribs++];
    va_attrib->type = VAConfigAttribRTFormat;
    va_status = vaGetConfigAttributes(vactx->display, profile, entrypoint,
        va_attribs, num_va_attribs);
    if (!va_check_status(va_status, "vaGetConfigAttributes()"))
        return vaapi_to_ffmpeg_error(va_status);

    va_attrib = &va_attribs[0];
    if (va_attrib->value == VA_ATTRIB_NOT_SUPPORTED ||
        !(va_attrib->value & VA_RT_FORMAT_YUV420))
        goto error_unsupported_chroma_format;
    va_attrib->value = VA_RT_FORMAT_YUV420;

    va_status = vaCreateConfig(vactx->display, profile, entrypoint,
        va_attribs, num_va_attribs, &va_config);
    if (!va_check_status(va_status, "vaCreateConfig()"))
        return vaapi_to_ffmpeg_error(va_status);

    static const int SCRATCH_SURFACES = 4;
    ret = vaapi_ensure_surfaces(dec, avctx->refs + 1 + SCRATCH_SURFACES);
    if (ret != 0)
        goto error_cleanup;

    va_surfaces = malloc(dec->num_va_surfaces * sizeof(*va_surfaces));
    if (!va_surfaces)
        goto error_cleanup;

    va_status = vaCreateSurfaces(vactx->display,
        avctx->coded_width, avctx->coded_height, VA_RT_FORMAT_YUV420,
        dec->num_va_surfaces, va_surfaces);
    if (!va_check_status(va_status, "vaCreateSurfaces()"))
        goto error_cleanup;

    for (i = 0; i < dec->num_va_surfaces; i++) {
        FFVASurface * const s = &dec->va_surfaces[i];
        ffva_surface_init(s, va_surfaces[i], VA_RT_FORMAT_YUV420,
            avctx->coded_width, avctx->coded_height);
        dec->va_surfaces_queue[i] = s;
    }
    dec->va_surfaces_queue_head = 0;
    dec->va_surfaces_queue_tail = 0;

    va_status = vaCreateContext(vactx->display, va_config,
        avctx->coded_width, avctx->coded_height, VA_PROGRESSIVE,
        va_surfaces, dec->num_va_surfaces, &va_context);
    if (!va_check_status(va_status, "vaCreateContext()"))
        goto error_cleanup;

    vactx->config_id = va_config;
    vactx->context_id = va_context;
    free(va_surfaces);
    return 0;

    /* ERRORS */
error_unsupported_chroma_format:
    av_log(dec, AV_LOG_ERROR, "unsupported YUV 4:2:0 chroma format\n");
    return AVERROR(ENOTSUP);
error_cleanup:
    va_destroy_context(vactx->display, &va_context);
    va_destroy_config(vactx->display, &va_config);
    if (ret == 0)
        ret = vaapi_to_ffmpeg_error(va_status);
    free(va_surfaces);
    return ret;
}

// Assigns a VA surface to the supplied AVFrame
static inline void
vaapi_set_frame_surface(AVCodecContext *avctx, AVFrame *frame, FFVASurface *s)
{
#if AV_NUM_DATA_POINTERS > 4
    frame->data[5] = (uint8_t *)s;
#endif
}

// Returns the VA surface object from an AVFrame
static FFVASurface *
vaapi_get_frame_surface(AVCodecContext *avctx, AVFrame *frame)
{
#if AV_NUM_DATA_POINTERS > 4
    return (FFVASurface *)frame->data[5];
#else
    FFVADecoder * const dec = avctx->opaque;
    VASurfaceID va_surface;
    uint32_t i;

    va_surface = (uintptr_t)frame->data[3];
    if (va_surface == VA_INVALID_ID)
        return NULL;

    for (i = 0; i < dec->num_va_surfaces; i++) {
        FFVASurface * const s = &dec->va_surfaces[i];
        if (s->id == va_surface)
            return s;
    }
    return NULL;
#endif
}

// AVCodecContext.get_format() implementation for VA-API
static enum AVPixelFormat
vaapi_get_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts)
{
    FFVADecoder * const dec = avctx->opaque;
    VAProfile profiles[3];
    uint32_t i, num_profiles;

    // Find a VA format
    for (i = 0; pix_fmts[i] != AV_PIX_FMT_NONE; i++) {
        if (pix_fmts[i] == AV_PIX_FMT_VAAPI)
            break;
    }
    if (pix_fmts[i] == AV_PIX_FMT_NONE)
        return AV_PIX_FMT_NONE;

    // Find a suitable VA profile that fits FFmpeg config
    num_profiles = 0;
    if (!ffmpeg_to_vaapi_profile(avctx->codec_id, avctx->profile,
            &profiles[num_profiles]))
        return AV_PIX_FMT_NONE;

    switch (profiles[num_profiles++]) {
    case VAProfileMPEG2Simple:
        profiles[num_profiles++] = VAProfileMPEG2Main;
        break;
    case VAProfileMPEG4Simple:
        profiles[num_profiles++] = VAProfileMPEG4AdvancedSimple;
        // fall-through
    case VAProfileMPEG4AdvancedSimple:
        profiles[num_profiles++] = VAProfileMPEG4Main;
        break;
    case VAProfileH264ConstrainedBaseline:
        profiles[num_profiles++] = VAProfileH264Main;
        // fall-through
    case VAProfileH264Main:
        profiles[num_profiles++] = VAProfileH264High;
        break;
    case VAProfileVC1Simple:
        profiles[num_profiles++] = VAProfileVC1Main;
        // fall-through
    case VAProfileVC1Main:
        profiles[num_profiles++] = VAProfileVC1Advanced;
        break;
    default:
        break;
    }
    for (i = 0; i < num_profiles; i++) {
        if (vaapi_has_config(dec, profiles[i], VAEntrypointVLD))
            break;
    }
    if (i == num_profiles)
        return AV_PIX_FMT_NONE;
    if (vaapi_init_decoder(dec, profiles[i], VAEntrypointVLD) < 0)
        return AV_PIX_FMT_NONE;
    return AV_PIX_FMT_VAAPI;
}

// Common initialization of AVFrame fields for VA-API purposes
static void
vaapi_get_buffer_common(AVCodecContext *avctx, AVFrame *frame, FFVASurface *s)
{
    memset(frame->data, 0, sizeof(frame->data));
    frame->data[0] = (uint8_t *)(uintptr_t)s->id;
    frame->data[3] = (uint8_t *)(uintptr_t)s->id;
    memset(frame->linesize, 0, sizeof(frame->linesize));
    frame->linesize[0] = avctx->coded_width; /* XXX: 8-bit per sample only */
    vaapi_set_frame_surface(avctx, frame, s);
}

#if AV_FEATURE_AVFRAME_REF
// AVCodecContext.get_buffer2() implementation for VA-API
static int
vaapi_get_buffer2(AVCodecContext *avctx, AVFrame *frame, int flags)
{
    FFVADecoder * const dec = avctx->opaque;
    FFVASurface *s;
    AVBufferRef *buf;
    int ret;

    if (!(avctx->codec->capabilities & CODEC_CAP_DR1))
        return avcodec_default_get_buffer2(avctx, frame, flags);

    ret = vaapi_acquire_surface(dec, &s);
    if (ret != 0)
        return ret;

    buf = av_buffer_create((uint8_t *)s, 0,
        (void (*)(void *, uint8_t *))vaapi_release_surface, dec,
        AV_BUFFER_FLAG_READONLY);
    if (!buf) {
        vaapi_release_surface(dec, s);
        return AVERROR(ENOMEM);
    }
    frame->buf[0] = buf;

    vaapi_get_buffer_common(avctx, frame, s);
    return 0;
}
#else
// AVCodecContext.get_buffer() implementation for VA-API
static int
vaapi_get_buffer(AVCodecContext *avctx, AVFrame *frame)
{
    FFVADecoder * const dec = avctx->opaque;
    FFVASurface *s;
    int ret;

    ret = vaapi_acquire_surface(dec, &s);
    if (ret != 0)
        return ret;

    frame->opaque = NULL;
    frame->type = FF_BUFFER_TYPE_USER;
    frame->reordered_opaque = avctx->reordered_opaque;

    vaapi_get_buffer_common(avctx, frame, s);
    return 0;
}

// AVCodecContext.reget_buffer() implementation for VA-API
static int
vaapi_reget_buffer(AVCodecContext *avctx, AVFrame *frame)
{
    assert(0 && "FIXME: implement AVCodecContext::reget_buffer() [VA-API]");

    // XXX: this is not the correct implementation
    return avcodec_default_reget_buffer(avctx, frame);
}

// AVCodecContext.release_buffer() implementation for VA-API
static void
vaapi_release_buffer(AVCodecContext *avctx, AVFrame *frame)
{
    FFVADecoder * const dec = avctx->opaque;
    FFVASurface * const s = vaapi_get_frame_surface(avctx, frame);

    memset(frame->data, 0, sizeof(frame->data));
    if (s && vaapi_release_surface(dec, s) != 0)
        return;
}
#endif

// Initializes AVCodecContext for VA-API decoding purposes
static void
vaapi_init_context(FFVADecoder *dec)
{
    AVCodecContext * const avctx = dec->avctx;

    avctx->hwaccel_context = &dec->va_context;
    avctx->thread_count = 1;
    avctx->draw_horiz_band = 0;
    avctx->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

    avctx->get_format = vaapi_get_format;
#if AV_FEATURE_AVFRAME_REF
    avctx->get_buffer2 = vaapi_get_buffer2;
#else
    avctx->get_buffer = vaapi_get_buffer;
    avctx->reget_buffer = vaapi_reget_buffer;
    avctx->release_buffer = vaapi_release_buffer;
#endif
}

// Initializes decoder for VA-API purposes, e.g. creates the VA display
static void
vaapi_init(FFVADecoder *dec)
{
    struct vaapi_context * const vactx = &dec->va_context;

    memset(vactx, 0, sizeof(*vactx));
    vactx->config_id = VA_INVALID_ID;
    vactx->context_id = VA_INVALID_ID;
    vactx->display = dec->display->va_display;
}

// Destroys all VA-API related resources
static void
vaapi_finalize(FFVADecoder *dec)
{
    struct vaapi_context * const vactx = &dec->va_context;
    uint32_t i;

    if (vactx->display) {
        va_destroy_context(vactx->display, &vactx->context_id);
        va_destroy_config(vactx->display, &vactx->config_id);
        if (dec->va_surfaces) {
            for (i = 0; i < dec->num_va_surfaces; i++)
                va_destroy_surface(vactx->display, &dec->va_surfaces[i].id);
        }
    }
    free(dec->va_surfaces);
    dec->num_va_surfaces = 0;
    free(dec->va_surfaces_queue);
    dec->va_surfaces_queue_length = 0;
    free(dec->va_profiles);
    dec->num_va_profiles = 0;
}

/* ------------------------------------------------------------------------ */
/* --- Base Decoder (SW)                                                --- */
/* ------------------------------------------------------------------------ */

static const AVClass *
ffva_decoder_class(void)
{
    static const AVClass g_class = {
        .class_name     = "FFVADecoder",
        .item_name      = av_default_item_name,
        .option         = NULL,
        .version        = LIBAVUTIL_VERSION_INT,
    };
    return &g_class;
}

static void
decoder_init_context(FFVADecoder *dec, AVCodecContext *avctx)
{
    dec->avctx = avctx;
    avctx->opaque = dec;
    vaapi_init_context(dec);
}

static int
decoder_init(FFVADecoder *dec, FFVADisplay *display)
{
    dec->klass = ffva_decoder_class();
    av_register_all();

    dec->display = display;
    vaapi_init(dec);
    dec->state = STATE_INITIALIZED;
    return 0;
}

static void
decoder_finalize(FFVADecoder *dec)
{
    ffva_decoder_close(dec);
    vaapi_finalize(dec);
}

static int
decoder_open(FFVADecoder *dec, const char *filename)
{
    AVFormatContext *fmtctx;
    AVCodecContext *avctx;
    AVCodec *codec;
    char errbuf[BUFSIZ];
    int i, ret;

    if (dec->state & STATE_OPENED)
        return 0;

    // Open and identify media file
    ret = avformat_open_input(&dec->fmtctx, filename, NULL, NULL);
    if (ret != 0)
        goto error_open_file;
    ret = avformat_find_stream_info(dec->fmtctx, NULL);
    if (ret < 0)
        goto error_identify_file;
    av_dump_format(dec->fmtctx, 0, filename, 0);
    fmtctx = dec->fmtctx;

    // Find the video stream and identify the codec
    for (i = 0; i < fmtctx->nb_streams; i++) {
        if (fmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
            !dec->stream)
            dec->stream = fmtctx->streams[i];
        else
            fmtctx->streams[i]->discard = AVDISCARD_ALL;
    }
    if (!dec->stream)
        goto error_no_video_stream;

    avctx = dec->stream->codec;
    decoder_init_context(dec, avctx);

    codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec)
        goto error_no_codec;
    ret = avcodec_open2(avctx, codec, NULL);
    if (ret < 0)
        goto error_open_codec;

    dec->frame = av_frame_alloc();
    if (!dec->frame)
        goto error_alloc_frame;

    dec->state |= STATE_OPENED;
    return 0;

    /* ERRORS */
error_open_file:
    av_log(dec, AV_LOG_ERROR, "failed to open file `%s': %s\n", filename,
        ffmpeg_strerror(ret, errbuf));
    return ret;
error_identify_file:
    av_log(dec, AV_LOG_ERROR, "failed to identify file `%s': %s\n", filename,
        ffmpeg_strerror(ret, errbuf));
    return ret;
error_no_video_stream:
    av_log(dec, AV_LOG_ERROR, "failed to find a video stream\n");
    return AVERROR_STREAM_NOT_FOUND;
error_no_codec:
    av_log(dec, AV_LOG_ERROR, "failed to find codec info for codec %d\n",
        avctx->codec_id);
    return AVERROR_DECODER_NOT_FOUND;
error_open_codec:
    av_log(dec, AV_LOG_ERROR, "failed to open codec %d\n", avctx->codec_id);
    return ret;
error_alloc_frame:
    av_log(dec, AV_LOG_ERROR, "failed to allocate video frame\n");
    return AVERROR(ENOMEM);
}

static void
decoder_close(FFVADecoder *dec)
{
    ffva_decoder_stop(dec);

    if (dec->avctx) {
        avcodec_close(dec->avctx);
        dec->avctx = NULL;
    }

    if (dec->fmtctx) {
        avformat_close_input(&dec->fmtctx);
        dec->fmtctx = NULL;
    }
    av_frame_free(&dec->frame);

    dec->state &= ~STATE_OPENED;
}

static int
handle_frame(FFVADecoder *dec, AVFrame *frame)
{
    FFVADecoderFrame * const dec_frame = &dec->decoded_frame;
    VARectangle * const crop_rect = &dec_frame->crop_rect;
    int data_offset;

    dec_frame->frame = frame;
    dec_frame->surface = vaapi_get_frame_surface(dec->avctx, frame);
    if (!dec_frame->surface)
        return AVERROR(EFAULT);

    data_offset = frame->data[0] - frame->data[3];
    dec_frame->has_crop_rect = data_offset > 0   ||
        frame->width  != dec->avctx->coded_width ||
        frame->height != dec->avctx->coded_height;
    crop_rect->x = data_offset % frame->linesize[0];
    crop_rect->y = data_offset / frame->linesize[0];
    crop_rect->width = frame->width;
    crop_rect->height = frame->height;
    return 0;
}

static int
decode_packet(FFVADecoder *dec, AVPacket *packet, int *got_frame_ptr)
{
    char errbuf[BUFSIZ];
    int got_frame, ret;

    if (!got_frame_ptr)
        got_frame_ptr = &got_frame;

    ret = avcodec_decode_video2(dec->avctx, dec->frame, got_frame_ptr, packet);
    if (ret < 0)
        goto error_decode_frame;
    if (*got_frame_ptr)
        return handle_frame(dec, dec->frame);
    return AVERROR(EAGAIN);

    /* ERRORS */
error_decode_frame:
    av_log(dec, AV_LOG_ERROR, "failed to decode frame: %s\n",
        ffmpeg_strerror(ret, errbuf));
    return ret;
}

static int
decoder_run(FFVADecoder *dec)
{
    AVPacket packet;
    char errbuf[BUFSIZ];
    int got_frame, ret;

    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    do {
        // Read frame from file
        ret = av_read_frame(dec->fmtctx, &packet);
        if (ret == AVERROR_EOF)
            break;
        else if (ret < 0)
            goto error_read_frame;

        // Decode video packet
        if (packet.stream_index == dec->stream->index)
            ret = decode_packet(dec, &packet, NULL);
        else
            ret = AVERROR(EAGAIN);
        av_free_packet(&packet);
    } while (ret == AVERROR(EAGAIN));
    if (ret == 0)
        return 0;

    // Decode cached frames
    packet.data = NULL;
    packet.size = 0;
    ret = decode_packet(dec, &packet, &got_frame);
    if (ret == AVERROR(EAGAIN) && !got_frame)
        ret = AVERROR_EOF;
    return ret;

    /* ERRORS */
error_read_frame:
    av_log(dec, AV_LOG_ERROR, "failed to read frame: %s\n",
        ffmpeg_strerror(ret, errbuf));
    return ret;
}

static int
decoder_flush(FFVADecoder *dec)
{
    return 0;
}

static int
decoder_start(FFVADecoder *dec)
{
    if (dec->state & STATE_STARTED)
        return 0;
    if (!(dec->state & STATE_OPENED))
        return AVERROR_UNKNOWN;

    dec->state |= STATE_STARTED;
    return 0;
}

static int
decoder_stop(FFVADecoder *dec)
{
    if (!(dec->state & STATE_STARTED))
        return 0;

    dec->state &= ~STATE_STARTED;
    return 0;
}

static int
decoder_get_frame(FFVADecoder *dec, FFVADecoderFrame **out_frame_ptr)
{
    FFVADecoderFrame *frame = NULL;
    int ret;

    if (!(dec->state & STATE_STARTED)) {
        ret = decoder_start(dec);
        if (ret < 0)
            return ret;
    }

    ret = decoder_run(dec);
    if (ret == 0)
        frame = &dec->decoded_frame;

    if (out_frame_ptr)
        *out_frame_ptr = frame;
    return ret;
}

static void
decoder_put_frame(FFVADecoder *dec, FFVADecoderFrame *frame)
{
    if (!frame || frame != &dec->decoded_frame)
        return;
}

/* ------------------------------------------------------------------------ */
/* --- Interface                                                        --- */
/* ------------------------------------------------------------------------ */

// Creates a new decoder instance
FFVADecoder *
ffva_decoder_new(FFVADisplay *display)
{
    FFVADecoder *dec;

    if (!display)
        return NULL;

    dec = calloc(1, sizeof(*dec));
    if (!dec)
        return NULL;
    if (decoder_init(dec, display) != 0)
        goto error;
    return dec;

error:
    ffva_decoder_free(dec);
    return NULL;
}

// Destroys the supplied decoder instance
void
ffva_decoder_free(FFVADecoder *dec)
{
    if (!dec)
        return;
    decoder_finalize(dec);
    free(dec);
}

/// Releases decoder instance and resets the supplied pointer to NULL
void
ffva_decoder_freep(FFVADecoder **dec_ptr)
{
    if (!dec_ptr)
        return;
    ffva_decoder_free(*dec_ptr);
    *dec_ptr = NULL;
}

// Initializes the decoder instance for the supplied video file by name
int
ffva_decoder_open(FFVADecoder *dec, const char *filename)
{
    if (!dec || !filename)
        return AVERROR(EINVAL);
    return decoder_open(dec, filename);
}

// Destroys the decoder resources used for processing the previous file
void
ffva_decoder_close(FFVADecoder *dec)
{
    if (!dec)
        return;
    decoder_close(dec);
}

// Starts processing the video file that was previously opened
int
ffva_decoder_start(FFVADecoder *dec)
{
    if (!dec)
        return AVERROR(EINVAL);
    return decoder_start(dec);
}

// Stops processing the active video file
void
ffva_decoder_stop(FFVADecoder *dec)
{
    if (!dec)
        return;
    decoder_stop(dec);
}

// Flushes any source data to be decoded
int
ffva_decoder_flush(FFVADecoder *dec)
{
    if (!dec)
        return AVERROR(EINVAL);
    return decoder_flush(dec);
}

// Returns some media info from an opened file
bool
ffva_decoder_get_info(FFVADecoder *dec, FFVADecoderInfo *info)
{
    if (!dec || !info)
        return false;
    if (!dec->avctx)
        return false;

    AVCodecContext * const avctx = dec->avctx;
    info->codec         = avctx->codec_id;
    info->profile       = avctx->profile;
    info->width         = avctx->width;
    info->height        = avctx->height;
    return true;
}

// Acquires the next decoded frame
int
ffva_decoder_get_frame(FFVADecoder *dec, FFVADecoderFrame **out_frame_ptr)
{
    if (!dec || !out_frame_ptr)
        return AVERROR(EINVAL);
    return decoder_get_frame(dec, out_frame_ptr);
}

// Releases the decoded frame back to the decoder for future use
void
ffva_decoder_put_frame(FFVADecoder *dec, FFVADecoderFrame *frame)
{
    if (!dec || !frame)
        return;
    decoder_put_frame(dec, frame);
}
