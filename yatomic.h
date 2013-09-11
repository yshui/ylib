/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013 Yuxuan Shui, yshuiv7@gmail.com */

/* No barriers guarenteed for atomic operations */

#pragma once

#include "macros.h"

#if __has_feature(c_atomic) || GCC_CHECK_VERSION(4, 9)

# include <stdatomic.h>
typedef _Atomic(int32_t) atomic_t;
# define atomic_typedef(type, name) typedef volatile _Atomic(type) name
# define atomic_get(x) (atomic_load(x))
# define atomic_set(x, v) (atomic_store(x, v))
# define atomic_fetch_inc(x) (atomic_fetch_add(x, 1))
# define atomic_fetch_dec(x) (atomic_fetch_add(x, -1))
# define atomic_init(x) (*(x) = ATOMIC_VAR_INIT(0))
# define atomic_xchg(ptr, n) (atomic_exchange(ptr, n))
# define atomic_cas(ptr, exp, n) (atomic_compare_exchange_weak(ptr, exp, n))

#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

typedef volatile int32_t atomic_t;
# define atomic_typedef(type, name) typedef volatile type name
# define atomic_get(x) (*x)
# define atomic_set(x, v) (*(x) = v)
# define atomic_fetch_inc(x) (__sync_fetch_and_add(x, 1))
# define atomic_fetch_dec(x) (__sync_fetch_and_sub(x, 1))
# define atomic_init(x) (*(x) = 0)
# ifdef __ATMOIC_SEQ_CST
#  define atomic_xchg(ptr, n) (__atomic_exchange_n(ptr, n, __ATOMIC_SEQ_CST))
#  define atomic_cas(ptr, exp, n) (__atomic_compare_exchange_n(ptr, exp, n, true, _ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
# else
#  define atomic_xchg(ptr, n) GNUC_EXT { \
		typeof(*ptr) __val; \
		do{ \
			__val = *ptr; \
		}while(!__sync_bool_compare_and_swap(ptr, __val, n)); \
		__val; \
	}
#  define atomic_cas(ptr, exp, n) GNUC_EXT { \
		typeof(*exp) __exp = *exp; \
		*exp = __sync_val_compare_and_swap(ptr, __exp, n); \
		__exp == *exp; \
	}
# endif
#else

/* No atomic support from compiler */
/* XXX use locks to gurantee atomic behavior */
typedef volatile int32_t atomic_t;
# define atomic_typedef(type, name) typedef volatile type name
# define atomic_get(x) (*(x))
# define atomic_set(x, v) (*(x) = v)
# define atomic_fetch_inc(x) (*(x)++)
# define atomic_fetch_dec(x) (*(x)--)
# define atomic_init(x) (*(x)=0)

#endif

