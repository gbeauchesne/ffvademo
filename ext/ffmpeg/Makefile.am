# Makefile.am - Rules for the built-in FFmpeg sources
#
# Copyright (C) 2014 Intel Corporation
#   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1
# of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 

lib_LTLIBRARIES =
if ENABLE_BUILTIN_FFMPEG
lib_LTLIBRARIES += libffva_ffmpeg.la
endif

ffmpeg_upstream	  = upstream
ffmpeg_srcdir	  = $(srcdir)/$(ffmpeg_upstream)
ffmpeg_builddir	  = $(builddir)/objs
ffmpeg_config_mak = $(ffmpeg_builddir)/config.mak
ffmpeg_extralibs  = test -f $(ffmpeg_config_mak) &&
ffmpeg_extralibs += $(SED) -n "/EXTRALIBS *= *\(.*\)/s//\1/p" $(ffmpeg_config_mak)

ffmpeg_libs = \
	-Wl,-Bsymbolic \
	-Wl,--whole-archive \
	-Wl,$(ffmpeg_builddir)/libavcodec/libavcodec.a \
	-Wl,$(ffmpeg_builddir)/libavformat/libavformat.a \
	-Wl,$(ffmpeg_builddir)/libavutil/libavutil.a \
	-Wl,--no-whole-archive \
	-Wl,--version-script,$(srcdir)/libffva_ffmpeg.vers \
	`$(ffmpeg_extralibs) | $(SED) -e "s/-l/-Wl,-l/g"` \
	$(NULL)

libffva_ffmpeg_la_SOURCES		=
nodist_EXTRA_libffva_ffmpeg_la_SOURCES	= dummy.c
libffva_ffmpeg_la_DEPENDENCIES		= ffmpeg.build
libffva_ffmpeg_la_LINK			= $(LINK) $(ffmpeg_libs)

FFMPEG_DEBUG	= yes
FFMPEG_TOOL	= disable
FFMPEG_PLAYER	= disable
FFMPEG_PROBE	= disable
FFMPEG_FILTERS	=
FFMPEG_DEMUXERS	=
FFMPEG_PARSERS	=
FFMPEG_DECODERS	=
FFMPEG_HWACCELS	=

# Containers
FFMPEG_DEMUXERS	+= asf
FFMPEG_DEMUXERS	+= avi
FFMPEG_DEMUXERS	+= flv
FFMPEG_DEMUXERS	+= ivf
FFMPEG_DEMUXERS	+= m4v
FFMPEG_DEMUXERS	+= matroska
FFMPEG_DEMUXERS	+= mov
FFMPEG_DEMUXERS	+= mpegps
FFMPEG_DEMUXERS	+= mpegts
FFMPEG_DEMUXERS	+= mpegtsraw
FFMPEG_DEMUXERS	+= rawvideo
FFMPEG_DEMUXERS	+= yuv4mpegpipe

# Video codecs
FFMPEG_DECODERS	+= rawvideo
FFMPEG_DECODERS	+= mpeg2video
FFMPEG_DEMUXERS	+= mpegvideo
FFMPEG_PARSERS	+= mpegvideo
FFMPEG_DECODERS	+= mpeg4
FFMPEG_PARSERS	+= mpeg4video
FFMPEG_DECODERS	+= h263
FFMPEG_DEMUXERS	+= h263
FFMPEG_PARSERS	+= h263
FFMPEG_DECODERS	+= h264
FFMPEG_DEMUXERS	+= h264
FFMPEG_FILTERS	+= h264_mp4toannexb
FFMPEG_PARSERS	+= h264
FFMPEG_DECODERS	+= hevc
FFMPEG_DEMUXERS	+= hevc
FFMPEG_PARSERS	+= hevc
FFMPEG_DECODERS	+= wmv3
FFMPEG_DECODERS	+= vc1
FFMPEG_DEMUXERS	+= vc1
FFMPEG_PARSERS	+= vc1
FFMPEG_DECODERS	+= vp8
FFMPEG_PARSERS	+= vp8
FFMPEG_DECODERS	+= vp9

# Hardware accelerated codecs
FFMPEG_HWACCELS	+= h263_vaapi
FFMPEG_HWACCELS	+= h264_vaapi
FFMPEG_HWACCELS	+= mpeg2_vaapi
FFMPEG_HWACCELS	+= mpeg4_vaapi
FFMPEG_HWACCELS	+= vc1_vaapi
FFMPEG_HWACCELS	+= wmv3_vaapi
ffmpeg_libs	+= $(LIBVA_LIBS)

clean-local: ffmpeg.clean
maintainer-clean-local: ffmpeg.maintainer.clean

ffmpeg.build: ffmpeg.configure.stamp
	@$(MAKE) -C $(ffmpeg_builddir)

ffmpeg.clean:
	@[ -d $(ffmpeg_builddir) ] && \
	$(MAKE) -C $(ffmpeg_builddir) clean || :
	rm -f ffmpeg.build.stamp ffmpeg.configure.stamp

ffmpeg.maintainer.clean: ffmpeg.clean
	rm -rf $(ffmpeg_builddir)

ffmpeg.configure.stamp:
	@[ -d $(ffmpeg_builddir) ] || $(MKDIR_P) $(ffmpeg_builddir);	 \
	cd $(ffmpeg_builddir)						;  \
	test "$(FFMPEG_DEBUG)" = "yes"					&& \
	CONFIGURE_FLAGS="$$CONFIGURE_FLAGS --enable-debug"		; \
	ff_filters="`echo '$(FFMPEG_FILTERS)' | $(SED) -e 's/ /,/g'`"	; \
	ff_decoders="`echo '$(FFMPEG_DECODERS)' | $(SED) -e 's/ /,/g'`"	; \
	ff_demuxers="`echo '$(FFMPEG_DEMUXERS)' | $(SED) -e 's/ /,/g'`"	; \
	ff_hwaccels="`echo '$(FFMPEG_HWACCELS)' | $(SED) -e 's/ /,/g'`"	; \
	ff_parsers="`echo '$(FFMPEG_PARSERS)' | $(SED) -e 's/ /,/g'`"	; \
	$(abs_srcdir)/$(ffmpeg_upstream)/configure $$CONFIGURE_FLAGS	\
		--cc='$(CC)'						\
		--ld='$(CCLD)'						\
		--enable-static						\
		--enable-pic						\
		--disable-shared					\
		--disable-all						\
		--disable-doc						\
		--$(FFMPEG_TOOL)-ffmpeg					\
		--$(FFMPEG_PLAYER)-ffplay				\
		--$(FFMPEG_PROBE)-ffprobe				\
		--enable-avutil						\
		--enable-avformat					\
		--enable-avcodec					\
		--disable-vdpau						\
		--enable-vaapi						\
		--enable-bsf="$$ff_filters"				\
		--enable-decoder="$$ff_decoders"			\
		--enable-demuxer="$$ff_demuxers"			\
		--enable-hwaccel="$$ff_hwaccels"			\
		--enable-parser="$$ff_parsers"				\
		--enable-protocol="file"				\
		--enable-runtime-cpudetect				&& \
	cd ..								&& \
	touch $@

CLEANFILES = ffmpeg.build.stamp

# Files for packaging
include $(srcdir)/sources.frag

ffmpeg_sources = \
	$(ffmpeg_srcdir)/COPYING.LGPLv2.1 \
	$(ffmpeg_srcdir)/Makefile \
	$(ffmpeg_srcdir)/RELEASE \
	$(ffmpeg_srcdir)/VERSION \
	$(ffmpeg_srcdir)/configure \
	$(ffmpeg_srcdir)/version.sh \
	$(ffmpeg_source_dirs:%=$(ffmpeg_srcdir)/%/Makefile) \
	$(ffmpeg_source_mak:%.mak=$(ffmpeg_srcdir)/%.mak) \
	$(ffmpeg_source_c:%.c=$(ffmpeg_srcdir)/%.c) \
	$(ffmpeg_source_h:%.h=$(ffmpeg_srcdir)/%.h) \
	$(ffmpeg_source_asm:%.asm=$(ffmpeg_srcdir)/%.asm)

EXTRA_DIST = \
	libffva_ffmpeg.vers	\
	sources.frag		\
	$(ffmpeg_sources)	\
	$(NULL)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in ffmpeg.configure.stamp
