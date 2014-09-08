bin_PROGRAMS		= ffvademo
noinst_LTLIBRARIES	= libffva.la

# -----------------------------------------------------------------------------
# --- FFmpeg decoder module                                                 ---
# -----------------------------------------------------------------------------

FFMPEG_CFLAGS	= $(LIBAVUTIL_CFLAGS) $(LIBAVFORMAT_CFLAGS) $(LIBAVCODEC_CFLAGS)
FFMPEG_LIBS	= $(LIBAVUTIL_LIBS) $(LIBAVFORMAT_LIBS) $(LIBAVCODEC_LIBS)

libffva_cflags = \
	$(FFMPEG_CFLAGS)	\
	$(LIBVA_CFLAGS)		\
	$(LIBVA_X11_CFLAGS)	\
	$(X11_CFLAGS)		\
	$(NULL)

libffva_libs = \
	$(FFMPEG_LIBS)		\
	$(LIBVA_LIBS)		\
	$(LIBVA_X11_LIBS)	\
	$(X11_LIBS)		\
	-lpthread		\
	$(NULL)

libffva_source_c = \
	ffmpeg_utils.c		\
	ffvadecoder.c		\
	ffvadisplay.c		\
	ffvasurface.c		\
	vaapi_utils.c		\
	$(NULL)

libffva_source_h = \
	ffmpeg_compat.h		\
	ffmpeg_utils.h		\
	ffvadecoder.h		\
	ffvadisplay.h		\
	ffvadisplay_priv.h	\
	ffvasurface.h		\
	vaapi_compat.h		\
	vaapi_utils.h		\
	$(NULL)

libffva_la_SOURCES		= $(libffva_source_c)
libffva_la_CFLAGS		= $(libffva_cflags)
libffva_la_LIBADD		= $(libffva_libs)

ffvademo_source_c		= ffvademo.c
ffvademo_SOURCES		= $(ffvademo_source_c)
ffvademo_CFLAGS			= $(libffva_cflags)
ffvademo_LDADD			= libffva.la

EXTRA_DIST = \
	$(libffva_source_c)	\
	$(libffva_source_h)	\
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in
