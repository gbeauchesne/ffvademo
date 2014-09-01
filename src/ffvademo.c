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
#include "ffmpeg_utils.h"

typedef struct {
    char *filename;
} Options;

typedef struct {
    const void *klass;
    Options options;
    FFVADisplay *display;
    VADisplay va_display;
} App;

#define OFFSET(x) offsetof(App, options.x)
static const AVOption app_options[] = {
    { "filename", "path to video file to decode", OFFSET(filename),
      AV_OPT_TYPE_STRING, },
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
app_run(App *app)
{
    const Options * const options = &app->options;

    if (!options->filename)
        goto error_no_filename;

    if (!app_ensure_display(app))
        return false;

    return true;

    /* ERRORS */
error_no_filename:
    av_log(app, AV_LOG_ERROR, "no video file specified on command line\n");
    return false;
}

static bool
app_parse_options(App *app, int argc, char *argv[])
{
    char errbuf[BUFSIZ];
    int ret, v, o = -1;

    static const struct option long_options[] = {
        { "help",           no_argument,        NULL, 'h'                   },
        { NULL, }
    };

    for (;;) {
        v = getopt_long(argc, argv, "-h", long_options, &o);
        if (v < 0)
            break;

        switch (v) {
        case '?':
            return false;
        case 'h':
            print_help(argv[0]);
            return false;
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
