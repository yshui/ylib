/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013 Yuxuan Shui, yshuiv7@gmail.com */

#pragma once

#include <stdlib.h>

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#if defined(__GNUC__) || defined(__clang__)
# define likely(expr) (__builtin_expect (!!(expr), 1))
# define unlikely(expr) (__builtin_expect (!!(expr), 0))
#else
# define likely(expr) (expr)
# define unlikely(expr) (expr)
#endif

#define GCC_CHECK_VERSION(major, minor) \
       (defined(__GNUC__) && \
        (__GNUC__ > (major) || \
         (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor))))

#if GCC_CHECK_VERSION(2, 8)
#  define GNUC_EXT __extension__
#else
#  define GNUC_EXT
#endif

#define container_of(ptr, type, member) GNUC_EXT ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
