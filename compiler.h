#pragma once

/* We only support for compiling with a compiler that
 * supports cleanup attribute
 */
#define Y_CLEANUP(x) __attribute__((cleanup(x)))

#if __STDC_VERSION__ >= 201112L
 #define Y_NORETURN _Noreturn
 #define Y_CTASSERT(x, y) static_assert(x, y)
 #define Y_CTASSERT_GLOBAL(x, y) static_assert(x, y)
#elif defined(__GNUC__)
 #define Y_NORETURN __attribute__((__noreturn__))
 #define Y_CTASSERT(x, y) static_assert(x, y)
 #define Y_CTASSERT_GLOBAL(x, y) static_assert(x, y)
#else
 #define Y_NORETURN
 #define __CTASSERT(x, y) \
	 typedef char __attribute__((unused)) \
	 __compile_time_assertion__ ## y[(x) ? 1 : -1]
 #define Y_CTASSERT(x, y) {__CTASSERT(x ,y);}
 #define Y_CTASSERT_GLOBAL(x, y) __CTASSERT(x, y)
#endif
