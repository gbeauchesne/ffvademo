/*
 * ffvadecoder.h - FFmpeg/vaapi decoder
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

#ifndef FFVA_DECODER_H
#define FFVA_DECODER_H

#include <stdint.h>
#include <va/va.h>
#include <libavcodec/avcodec.h>
#include "ffvadisplay.h"
#include "ffvasurface.h"

typedef struct ffva_decoder_s           FFVADecoder;
typedef struct ffva_decoder_info_s      FFVADecoderInfo;
typedef struct ffva_decoder_frame_s     FFVADecoderFrame;

struct ffva_decoder_info_s {
    int codec;
    int profile;
    int width;
    int height;
};

struct ffva_decoder_frame_s {
    AVFrame *frame;
    FFVASurface *surface;
    VARectangle crop_rect;
    bool has_crop_rect;
};

/** Creates a new decoder instance */
FFVADecoder *
ffva_decoder_new(FFVADisplay *display);

/** Destroys the supplied decoder instance */
void
ffva_decoder_free(FFVADecoder *dec);

/** Releases decoder instance and resets the supplied pointer to NULL */
void
ffva_decoder_freep(FFVADecoder **dec_ptr);

/** Initializes the decoder instance for the supplied video file by name */
int
ffva_decoder_open(FFVADecoder *dec, const char *filename);

/** Destroys the decoder resources used for processing the previous file */
void
ffva_decoder_close(FFVADecoder *dec);

/** Starts processing the video file that was previously opened */
int
ffva_decoder_start(FFVADecoder *dec);

/** Stops processing the active video file */
void
ffva_decoder_stop(FFVADecoder *dec);

/** Flushes any source data to be decoded */
int
ffva_decoder_flush(FFVADecoder *dec);

/** Returns some media info from an opened file */
bool
ffva_decoder_get_info(FFVADecoder *dec, FFVADecoderInfo *info);

/** Acquires the next decoded frame */
int
ffva_decoder_get_frame(FFVADecoder *dec, FFVADecoderFrame **out_frame_ptr);

/** Releases the decoded frame back to the decoder for future use */
void
ffva_decoder_put_frame(FFVADecoder *dec, FFVADecoderFrame *frame);

#endif /* FFVA_DECODER_H */
