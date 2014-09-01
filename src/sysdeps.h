/*
 * sysdeps.h - System-dependent definitions
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

#ifndef SYSDEPS_H
#define SYSDEPS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <libavutil/log.h>
#include <libavutil/mem.h>

/* Visibility attributes */
#if defined __GNUC__ && __GNUC__ >= 4
# define DLL_PUBLIC __attribute__((visibility("default")))
# define DLL_HIDDEN __attribute__((visibility("hidden")))
#else
# define DLL_PUBLIC
# define DLL_HIDDEN
#endif

/* Helper macros */
#define U_GEN_STRING(x)                 U_GEN_STRING_I(x)
#define U_GEN_STRING_I(x)               #x
#define U_GEN_CONCAT(a1, a2)            U_GEN_CONCAT2_I(a1, a2)
#define U_GEN_CONCAT2(a1, a2)           U_GEN_CONCAT2_I(a1, a2)
#define U_GEN_CONCAT2_I(a1, a2)         a1 ## a2
#define U_GEN_CONCAT3(a1, a2, a3)       U_GEN_CONCAT3_I(a1, a2, a3)
#define U_GEN_CONCAT3_I(a1, a2, a3)     a1 ## a2 ## a3
#define U_GEN_CONCAT4(a1, a2, a3, a4)   U_GEN_CONCAT4_I(a1, a2, a3, a4)
#define U_GEN_CONCAT4_I(a1, a2, a3, a4) a1 ## a2 ## a3 ## a4

#endif /* SYSDEPS_H */
