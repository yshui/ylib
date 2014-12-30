#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <uthash.h>

#include "ylist.h"
#include "yskiplist.h"
#include "ydef.h"
#include "compiler.h"
#ifndef Y_SINGLE_THREAD
#include "ythread.h"
#include "yatomic.h"
#endif

#define YREF_OWNER_RETURN ((void *)(-1))

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
	size_t len;
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
	UT_hash_handle hh;
};
#else
typedef struct yref_info {
	int ref_count;
	yref_dtor dtor;
} yref_t;
#endif

#define yref_def_scope_out(type, member) void type##_scope_out_func(type **p) {\
	_yref_unref(p, &(*p)->member); \
}

#define yref_def_ref(type, name) type *name Y_CLEANUP(type##_scope_out_func)

#define yref_ref(src, dst) _yref_ref(&(dst), src, NULL)

#define yref_move(src, dst)

#define yref_ref_return(ret)
#define yref_return_get(expr, dst)

/*
 * yref_uref: Unref an object, run the dtor if necessery
 * @pp: pointer to a referer
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
	assert(ref);
	assert(*ref->owner == p);
	HASH_DEL(r->referers, ref);
	mtx_unlock(&r->ref_mtx);
#endif
	tmp = yatomic_dec(&r->ref_count);
	assert(tmp >= 0);
	if (tmp == 0) {
		r->dtor(p);
		return true;
	}
	return false;
}

static inline void _yref_ref(void **pp, yref_t *r, yref_t *parent) {
	void *p = r->start;
	*pp = p;
#ifdef YREF_CHECK
	struct yref_entry *new_ref = talloc(1, struct yref_entry);
	new_ref->owner = pp;

	mtx_lock(&r->ref_mtx);
	++r->ref_count;
	HASH_ADD_PTR(r->referers, owner, new_ref);
	mtx_unlock(&r->ref_mtx);
#endif
	yatomic_inc(&r->ref_count);
}

static inline void yref_misuse_check(yref_t *r) {
	struct yref_entry *tmp, *ref;
	HASH_ITER(hh, r->referers, ref, tmp) {
		assert(*ref->owner == r->start);
	}
}
