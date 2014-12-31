#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <uthash.h>

#include "ydef.h"
#include "compiler.h"
#ifndef Y_SINGLE_THREAD
#include "ythread.h"
#include "yatomic.h"
#endif

typedef void (*yref_dtor)(void *);

#ifdef Y_SINGLE_THREAD
# define mtx_lock
# define mtx_init
# define mtx_unlock
#endif

#define YREF_CHECK
#ifdef YREF_CHECK
struct yref;

typedef struct yref_info {
	int ref_count;
	void *start;
	yref_dtor dtor;
	struct yref_entry *referers;
#ifndef Y_SINGLE_THREAD
	mtx_t ref_mtx;
#endif
} yref_t;

struct yref_entry {
	//owner = address of pointer to the ref counted obj
	void **owner;
	yref_t *info;
	bool ret;
	UT_hash_handle hh;
};
#else
typedef struct yref_info {
	int ref_count;
	yref_dtor dtor;
} yref_t;
#endif

typedef struct yref_ret {
	void **pp;
	yref_t *info;
} yref_ret_t;

#define yref_def_scope_out_proto(type) void type##_scope_out_func(type **);

#define yref_def_scope_out(type, member) void type##_scope_out_func(type **p) {\
	_yref_unref(p, &(*p)->member); \
}

#define yref_var(type, name) type *name Y_CLEANUP(type##_scope_out_func)

#define yref_ref(src, dst, member) _yref_ref(&(dst), &(src)->member)

#define yref_move(src, dst, member) _yref_move(&(dst), &(src), &(src)->member, false)

#define yref_ref_return(ret, member) { \
	_yref_mark_as_return(&(ret), &(ret)->member); \
	return ((yref_ret_t){&(ret), &(ret)->member}); \
}
#define yref_return_get(expr, dst, member) { \
	yref_ret_t __tmp = (expr); \
	_yref_move(__tmp.pp, (void **)&(dst), __tmp.info, true); \
	dst = __tmp.info->start; \
}

static inline void yref_init(void *p, yref_t *r, yref_dtor dtor) {
	yatomic_set(&r->ref_count, 0);
	r->dtor = dtor;
#ifdef YREF_CHECK
	r->start = p;
	mtx_init(&r->ref_mtx, 0);
	r->referers = NULL;
#else
	(void)p;
#endif
}

/*
 * _yref_mark_as_return: Mark a referer for returning,
 * sanity checks will not fail for referers marked as return.
 * @pp: pointer to the referer
 * @r: yref_t
 */
static inline void _yref_mark_as_return(void **pp, yref_t *r) {
#ifdef YREF_CHECK
	mtx_lock(&r->ref_mtx);
	struct yref_entry *ref;
	HASH_FIND_PTR(r->referers, &pp, ref);
	assert(!ref->ret);
	ref->ret = true;
	mtx_unlock(&r->ref_mtx);
#else
	(void)pp,
	(void)r;
#endif
}

/*
 * _yref_move: Transfer the ownership from one referer to another
 * @src: the source referer
 * @dst: the dest referer
 * @r: yref_t
 */
static inline void _yref_move(void **src, void **dst, yref_t *r,
			      bool ret) {
#ifdef YREF_CHECK
	mtx_lock(&r->ref_mtx);
	struct yref_entry *ref;
	HASH_FIND_PTR(r->referers, &src, ref);
	HASH_DEL(r->referers, ref);
	assert(ref);
	assert(ref->ret || *ref->owner == ref->info->start);
	ref->owner = dst;
	ref->ret = false;
	HASH_ADD_PTR(r->referers, owner, ref);
	mtx_unlock(&r->ref_mtx);
#endif
	if (!ret) {
		//if ret == true, src points to invalid stack area
		*dst = *src;
		*src = NULL;
	}
}

/*
 * yref_uref: Unref an object, run the dtor if necessery
 * @pp: pointer to the referer
 * @r: yref_t
 * @return: true if the object has zero reference.
 */
static inline bool _yref_unref(void **pp, yref_t *r) {
	void *p = *pp;
	int tmp;
	*pp = NULL;
#ifdef YREF_CHECK
	mtx_lock(&r->ref_mtx);
	struct yref_entry *ref;
	HASH_FIND_PTR(r->referers, &pp, ref);
	HASH_DEL(r->referers, ref);
	mtx_unlock(&r->ref_mtx);
	assert(ref);
	assert(*ref->owner == ref->info->start);
	free(ref);
#endif
	tmp = yatomic_dec(&r->ref_count);
	assert(tmp >= 0);
	if (tmp == 0) {
		r->dtor(p);
		return true;
	}
	return false;
}

/*
 * yref_ref: Reference an object
 * @pp: pointer to the referer
 * @r: yref_t
 */
static inline void _yref_ref(void **pp, yref_t *r) {
	void *p = r->start;
	*pp = p;
#ifdef YREF_CHECK
	struct yref_entry *new_ref = talloc(1, struct yref_entry);
	new_ref->owner = pp;

	mtx_lock(&r->ref_mtx);
	HASH_ADD_PTR(r->referers, owner, new_ref);
	mtx_unlock(&r->ref_mtx);
#endif
	yatomic_inc(&r->ref_count);
}

static inline void yref_misuse_check(yref_t *r) {
	struct yref_entry *tmp, *ref;
	mtx_lock(&r->ref_mtx);
	HASH_ITER(hh, r->referers, ref, tmp) {
		assert(*ref->owner == r->start);
	}
	mtx_unlock(&r->ref_mtx);
}
