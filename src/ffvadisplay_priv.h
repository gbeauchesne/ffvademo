/*
 * ffvadisplay_priv.h - VA display abstraction (private definitions)
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

#ifndef FFVA_DISPLAY_PRIV_H
#define FFVA_DISPLAY_PRIV_H

#include <va/va.h>

struct ffva_display_s {
    const void *klass;
    void *native_display;
    VADisplay va_display;
    char *display_name;
};

#endif /* FFVA_DISPLAY_PRIV_H */
