# FFmpeg/vaapi demo player
m4_define([ffva_major_version], [0])
m4_define([ffva_minor_version], [1])
m4_define([ffva_micro_version], [0])
m4_define([ffva_pre_version],   [1])
m4_define([ffva_version],
          [ffva_major_version.ffva_minor_version.ffva_micro_version])
m4_if(ffva_pre_version, [0], [], [
m4_append([ffva_version], ffva_pre_version, [.pre])
])

# Configure defaults
m4_define([default_builtin_ffmpeg],     [yes])

# FFmpeg version number
m4_define([libavutil_version],          [51.22.1])
m4_define([libavformat_version],        [53.21.1])
m4_define([libavcodec_version],         [53.35.0])

# VA-API minimum version number
m4_define([va_api_version],             [0.30.4])
m4_define([va_api_drm_version],         [0.33.0])
m4_define([va_api_x11_version],         [0.31.0])

# libva package version number
m4_define([libva_drm_package_version],  [1.1.0])
m4_define([libva_x11_package_version],  [1.0.3])

AC_PREREQ([2.66])
AC_INIT([ffvademo], [ffva_version],
    [gwenole.beauchesne@intel.com],
    [ffvademo])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.11 tar-ustar no-dist-gzip dist-bzip2])

TODAY="`LC_ALL=C date +'%a, %d %b %Y %X %z'`"
AC_SUBST([TODAY])

LIBVA_PACKAGE_VERSION=libva_x11_package_version
AC_SUBST([LIBVA_PACKAGE_VERSION])

TARGET_ARCH="$target_cpu"
AC_SUBST([TARGET_ARCH])

dnl Use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

dnl Configure options
AC_ARG_ENABLE([builtin-ffmpeg],
    AS_HELP_STRING([--enable-builtin-ffmpeg],
        [build built-in FFmpeg module @<:@default=default_builtin_ffmpeg@:>@]),
    [], [enable_builtin_ffmpeg=default_builtin_ffmpeg])

dnl Resolve dependencies
if test "$enable_builtin_ffmpeg" = "yes"; then
    needs_yasm="yes"
fi

dnl Check for tools
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

AC_ARG_VAR([YASM], [Path to yasm program, if any])
AC_PATH_PROG([YASM], [yasm])
if test "$needs_yasm" = "yes" -a -z "$YASM"; then
    AC_MSG_ERROR([yasm is needed to build FFmpeg sources])
fi

AC_C_BIGENDIAN
AC_CHECK_LIB([m], [log10])

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT

dnl FFVA_LT_LDFLAGS:
FFVA_LT_CURRENT="$FFVA_MAJOR_VERSION"
FFVA_LT_REV=ffva_lt_revision
FFVA_LT_AGE=ffva_lt_age
FFVA_LT_VERSION="$FFVA_LT_CURRENT:$FFVA_LT_REV:$FFVA_LT_AGE"
FFVA_LT_LDFLAGS="-version-info $FFVA_LT_VERSION"
AC_SUBST([FFVA_LT_VERSION])
AC_SUBST([FFVA_LT_LDFLAGS])

dnl ---------------------------------------------------------------------------
dnl -- Renderers                                                             --
dnl ---------------------------------------------------------------------------

dnl Renderer
FFVA_RENDERER="x11"
case "$FFVA_RENDERER" in
    x11)        FFVA_RENDERER_NAME="X11";;
    *) AC_MSG_ERROR([Unsupported renderer $FFVA_RENDERER]);;
esac
AC_SUBST([FFVA_RENDERER])

dnl Check for X11
USE_X11=0
if test "$FFVA_RENDERER" = "x11"; then
    PKG_CHECK_MODULES([X11], [x11])
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $X11_CFLAGS"
    AC_CHECK_HEADERS([X11/Xlib.h X11/Xutil.h X11/Xatom.h],
        [USE_X11=1], [AC_MSG_ERROR([incomplete libx11-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi
AC_DEFINE_UNQUOTED([USE_X11], [$USE_X11],
    [Defined to 1 if X11 renderer is enabled])
AM_CONDITIONAL([USE_X11], [test $USE_X11 -eq 1])

dnl ---------------------------------------------------------------------------
dnl -- VA-API                                                                --
dnl ---------------------------------------------------------------------------

dnl Core API
LIBVA_PKGNAME="libva"
PKG_CHECK_MODULES([LIBVA], [$LIBVA_PKGNAME >= va_api_version])
AC_SUBST([LIBVA_PKGNAME])

VA_VERSION=`$PKG_CONFIG --modversion libva`
VA_MAJOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f1`
VA_MINOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f2`
VA_MICRO_VERSION=`echo "$VA_VERSION" | cut -d'.' -f3`
VA_VERSION_STR="$VA_VERSION"

dnl VA/X11 API
USE_VA_X11=0
if test $USE_X11 -eq 1; then
    PKG_CHECK_MODULES([LIBVA_X11], [libva-x11 >= va_api_x11_version])
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$LIBVA_X11_CFLAGS"
    AC_CHECK_HEADERS([va/va_x11.h], [USE_VA_X11=1],
        [AC_MSG_ERROR([incomplete libva-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi
AC_DEFINE_UNQUOTED([USE_VA_X11], [$USE_VA_X11],
    [Defined to 1 if libva (X11) backend is enabled])
AM_CONDITIONAL([USE_VA_X11], [test $USE_VA_X11 -eq 1])

dnl Check for vpp (video post-processing) support
USE_VA_VPP=0
AC_CACHE_CHECK([for video post-postprocessing API],
    ac_cv_have_va_vpp_api, [
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBVA_CFLAGS"
    saved_LIBS="$LIBS"
    LIBS="$LIBS $LIBVA_LIBS"
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <va/va.h>
              #include <va/va_vpp.h>]],
            [[VADisplay va_dpy;
              VAContextID vpp_ctx;
              VAProcFilterType filters[VAProcFilterCount];
              unsigned int num_filters = VAProcFilterCount;
              vaQueryVideoProcFilters(va_dpy, vpp_ctx, filters, &num_filters);
              ]])],
        [ac_cv_have_va_vpp_api="yes" USE_VA_VPP=1],
        [ac_cv_have_va_vpp_api="no"]
    )
    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"
])
AC_DEFINE_UNQUOTED([USE_VA_VPP], [$USE_VA_VPP],
    [Defined to 1 if video post-processing is used])
AM_CONDITIONAL([USE_VA_VPP], [test $USE_VA_VPP -eq 1])

dnl ---------------------------------------------------------------------------
dnl -- FFmpeg                                                                --
dnl ---------------------------------------------------------------------------

dnl Check for libavutil
if test "$enable_builtin_ffmpeg" != "yes"; then
    PKG_CHECK_MODULES([LIBAVUTIL], [libavutil >= libavutil_version])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBAVUTIL_CFLAGS"
    AC_CHECK_HEADERS([libavutil/pixfmt.h libavutil/pixdesc.h],
        [], [AC_MSG_ERROR([incomplete libavutil-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for libavformat
if test "$enable_builtin_ffmpeg" != "yes"; then
    PKG_CHECK_MODULES([LIBAVFORMAT], [libavformat >= libavformat_version])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBAVFORMAT_CFLAGS"
    AC_CHECK_HEADERS([libavformat/avformat.h],
        [], [AC_MSG_ERROR([incomplete libavformat-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for libavcodec
if test "$enable_builtin_ffmpeg" != "yes"; then
    PKG_CHECK_MODULES([LIBAVCODEC], [libavcodec >= libavcodec_version])

    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBAVCODEC_CFLAGS"
    AC_CHECK_HEADERS([libavcodec/avcodec.h],
        [], [AC_MSG_ERROR([incomplete libavcodec-dev (headers) package])])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl ---------------------------------------------------------------------------
dnl -- Generate files and summary                                            --
dnl ---------------------------------------------------------------------------

AM_CONDITIONAL([ENABLE_BUILTIN_FFMPEG], [test "$enable_builtin_ffmpeg" = "yes"])

AC_CONFIG_FILES([
    Makefile
    ext/Makefile
    ext/ffmpeg/Makefile
    src/Makefile
])
AC_OUTPUT

dnl Print summary
echo
echo $PACKAGE configuration summary:
echo
echo Renderer ......................... : $FFVA_RENDERER_NAME
echo VA-API version ................... : $VA_VERSION_STR
