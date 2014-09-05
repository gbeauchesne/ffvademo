/*
 * ffvademo.c - FFmpeg/vaapi demo program
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

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include <getopt.h>
#include <libavutil/opt.h>
#include "ffvadisplay.h"
#include "ffvadecoder.h"
#include "ffvarenderer.h"
#include "ffmpeg_utils.h"

#if USE_X11
# include "ffvarenderer_x11.h"
#endif

// Default window size
#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480

// Default renderer
#define DEFAULT_RENDERER FFVA_RENDERER_TYPE_X11

typedef struct {
    char *filename;
    FFVARendererType renderer_type;
} Options;

typedef struct {
    const void *klass;
    Options options;
    FFVADisplay *display;
    VADisplay va_display;
    FFVADecoder *decoder;
    FFVARenderer *renderer;
    uint32_t renderer_width;
    uint32_t renderer_height;
} App;

#define OFFSET(x) offsetof(App, options.x)
static const AVOption app_options[] = {
    { "filename", "path to video file to decode", OFFSET(filename),
      AV_OPT_TYPE_STRING, },
    { "renderer", "renderer type to use", OFFSET(renderer_type),
      AV_OPT_TYPE_FLAGS, { .i64 = DEFAULT_RENDERER }, 0, INT_MAX, 0,
      "renderer" },
    { "x11", "X11", 0, AV_OPT_TYPE_CONST, { .i64 = FFVA_RENDERER_TYPE_X11 },
      0, 0, 0, "renderer" },
    { NULL, }
};

static void
app_free(App *app);

static const char *
get_basename(const char *filename)
{
    const char * const s = strrchr(filename, '/');

    return s ? s + 1 : filename;
}

static void
print_help(const char *prog)
{
    printf("Usage: %s <video>\n", get_basename(prog));
    printf("\n");
    printf("Options:\n");
    printf("  %-28s  display this help and exit\n",
           "-h, --help");
    printf("  %-28s  select a particular renderer (string) [default='x11']\n",
           "-r, --renderer=TYPE");
}

static const AVClass *
app_class(void)
{
    static const AVClass g_class = {
        .class_name     = "FFVADemo",
        .item_name      = av_default_item_name,
        .option         = app_options,
        .version        = LIBAVUTIL_VERSION_INT,
    };
    return &g_class;
}

static App *
app_new(void)
{
    App *app;

    app = calloc(1, sizeof(*app));
    if (!app)
        return NULL;

    app->klass = app_class();
    av_opt_set_defaults(app);
    return app;
}

static void
app_free(App *app)
{
    if (!app)
        return;

    ffva_renderer_freep(&app->renderer);
    ffva_decoder_freep(&app->decoder);
    ffva_display_freep(&app->display);
    av_opt_free(app);
    free(app);
}

static bool
app_ensure_display(App *app)
{
    if (!app->display) {
        app->display = ffva_display_new(NULL);
        if (!app->display)
            goto error_create_display;
        app->va_display = ffva_display_get_va_display(app->display);
    }
    return true;

    /* ERRORS */
error_create_display:
    av_log(app, AV_LOG_ERROR, "failed to create VA display\n");
    return false;
}

static bool
app_ensure_decoder(App *app)
{
    if (!app->decoder) {
        app->decoder = ffva_decoder_new(app->display);
        if (!app->decoder)
            goto error_create_decoder;
    }
    return true;

    /* ERRORS */
error_create_decoder:
    av_log(app, AV_LOG_ERROR, "failed to create FFmpeg/vaapi decoder\n");
    return false;
}

static bool
app_ensure_renderer(App *app)
{
    const Options * const options = &app->options;
    uint32_t flags = 0;

    if (!app->renderer) {
        switch (options->renderer_type) {
#if USE_X11
        case FFVA_RENDERER_TYPE_X11:
            app->renderer = ffva_renderer_x11_new(app->display, flags);
            break;
#endif
        }
        if (!app->renderer)
            goto error_create_renderer;
    }
    return true;

    /* ERRORS */
error_create_renderer:
    av_log(app, AV_LOG_ERROR, "failed to create renderer\n");
    return false;
}

static bool
app_ensure_renderer_size(App *app, uint32_t width, uint32_t height)
{
    if (!app_ensure_renderer(app))
        return false;

    if (app->renderer_width != width || app->renderer_height != height) {
        if (!ffva_renderer_set_size(app->renderer, width, height))
            return false;
        app->renderer_width = width;
        app->renderer_height = height;
    }
    return true;
}

static bool
app_render_surface(App *app, FFVASurface *s, const VARectangle *rect,
    uint32_t flags)
{
    if (!app_ensure_renderer_size(app, rect->width, rect->height))
        return false;

    return ffva_renderer_put_surface(app->renderer, s, rect, NULL, flags);
}

static int
app_render_frame(App *app, FFVADecoderFrame *dec_frame)
{
    FFVASurface * const s = dec_frame->surface;
    AVFrame * const frame = dec_frame->frame;
    const VARectangle *rect;
    VARectangle tmp_rect;
    uint32_t i, flags;

    if (dec_frame->has_crop_rect)
        rect = &dec_frame->crop_rect;
    else {
        tmp_rect.x = 0;
        tmp_rect.y = 0;
        tmp_rect.width = s->width;
        tmp_rect.height = s->height;
        rect = &tmp_rect;
    }

    flags = 0;
    for (i = 0; i < 1 + !!frame->interlaced_frame; i++) {
        flags &= ~(VA_TOP_FIELD|VA_BOTTOM_FIELD);
        if (frame->interlaced_frame) {
            flags |= ((i == 0) ^ !!frame->top_field_first) == 0 ?
                VA_TOP_FIELD : VA_BOTTOM_FIELD;
        }
        if (!app_render_surface(app, s, rect, flags))
            return AVERROR_UNKNOWN;
    }
    return 0;
}

static int
app_decode_frame(App *app)
{
    FFVADecoderFrame *dec_frame;
    int ret;

    ret = ffva_decoder_get_frame(app->decoder, &dec_frame);
    if (ret == 0) {
        ret = app_render_frame(app, dec_frame);
        ffva_decoder_put_frame(app->decoder, dec_frame);
    }
    return ret;
}

static bool
app_run(App *app)
{
    const Options * const options = &app->options;
    FFVADecoderInfo info;
    char errbuf[BUFSIZ];
    int ret;

    if (!options->filename)
        goto error_no_filename;

    if (!app_ensure_display(app))
        return false;
    if (!app_ensure_decoder(app))
        return false;
    if (!app_ensure_renderer(app))
        return false;

    if (ffva_decoder_open(app->decoder, options->filename) < 0)
        return false;
    if (ffva_decoder_start(app->decoder) < 0)
        return false;

    if (!ffva_decoder_get_info(app->decoder, &info))
        return false;

    do {
        ret = app_decode_frame(app);
    } while (ret == 0 || ret == AVERROR(EAGAIN));
    if (ret != AVERROR_EOF)
        goto error_decode_frame;
    ffva_decoder_stop(app->decoder);
    ffva_decoder_close(app->decoder);
    return true;

    /* ERRORS */
error_no_filename:
    av_log(app, AV_LOG_ERROR, "no video file specified on command line\n");
    return false;
error_decode_frame:
    av_log(app, AV_LOG_ERROR, "failed to decode frame: %s\n",
        ffmpeg_strerror(ret, errbuf));
    return false;
}

static bool
app_parse_options(App *app, int argc, char *argv[])
{
    char errbuf[BUFSIZ];
    int ret, v, o = -1;

    static const struct option long_options[] = {
        { "help",           no_argument,        NULL, 'h'                   },
        { "renderer",       required_argument,  NULL, 'r'                   },
        { NULL, }
    };

    for (;;) {
        v = getopt_long(argc, argv, "-hr:", long_options, &o);
        if (v < 0)
            break;

        switch (v) {
        case '?':
            return false;
        case 'h':
            print_help(argv[0]);
            return false;
        case 'r':
            ret = av_opt_set(app, "renderer", optarg, 0);
            break;
        case '\1':
            ret = av_opt_set(app, "filename", optarg, 0);
            break;
        default:
            ret = 0;
            break;
        }
        if (ret != 0)
            goto error_set_option;
    }
    return true;

    /* ERRORS */
error_set_option:
    if (o < 0) {
        av_log(app, AV_LOG_ERROR, "failed to set short option -%c: %s\n",
            v, ffmpeg_strerror(ret, errbuf));
    }
    else {
        av_log(app, AV_LOG_ERROR, "failed to set long option --%s: %s\n",
            long_options[o].name, ffmpeg_strerror(ret, errbuf));
    }
    return false;
}

int
main(int argc, char *argv[])
{
    App *app;
    int ret = EXIT_FAILURE;

    if (argc == 1) {
        print_help(argv[0]);
        return EXIT_SUCCESS;
    }

    app = app_new();
    if (!app || !app_parse_options(app, argc, argv) || !app_run(app))
        goto cleanup;
    ret = EXIT_SUCCESS;

cleanup:
    app_free(app);
    return ret;
}
