/*
 * egl_compat.h - EGL compatiliby layer
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

#ifndef EGL_COMPAT_H
#define EGL_COMPAT_H

#if USE_GLES_VERSION == 0
# define GL_GLEXT_PROTOTYPES 1
# include <GL/gl.h>
# include <GL/glext.h>
# define OPENGL_API             EGL_OPENGL_API
# define OPENGL_BIT             EGL_OPENGL_BIT
#elif USE_GLES_VERSION == 1
# include <GLES/gl.h>
# include <GLES/glext.h>
# define OPENGL_API             EGL_OPENGL_ES_API
# define OPENGL_BIT             EGL_OPENGL_ES_BIT
#elif USE_GLES_VERSION == 2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
# define OPENGL_API             EGL_OPENGL_ES_API
# define OPENGL_BIT             EGL_OPENGL_ES2_BIT
#elif USE_GLES_VERSION == 3
# include <GLES3/gl3.h>
# include <GLES3/gl3ext.h>
# include <GLES2/gl2ext.h>
# define OPENGL_API             EGL_OPENGL_ES_API
# define OPENGL_BIT             EGL_OPENGL_ES3_BIT_KHR
#else
# error "Unsupported GLES API version"
#endif

#ifndef GL_R8
#define GL_R8                   GL_R8_EXT
#endif
#ifndef GL_RG8
#define GL_RG8                  GL_RG8_EXT
#endif

#endif /* EGL_COMPAT_H */
