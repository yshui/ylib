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
#ifdef YREF_CHECK
	int ref_count;
#else
	atomic_t ref_count;
#endif
	void *start;
	size_t len;
	int visited;
	yref_dtor dtor;
	struct ylist_head children;
	struct yref *referers;
#ifndef Y_SINGLE_THREAD
	mtx_t ref_mtx, child_mtx;
#endif
} yref_t;

struct yref {
	//owner = address of pointer to the ref counted obj
	//parent = a ref counted obj that contains the owner
	void **owner;
	yref_t *parent, *info;
	struct ylist_head children;
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
#define yref_ref_member(src, dst, parent) _yref_ref(&(dst), src, parent)

#define yref_move(src, dst)
#define yref_move_member(src, dst, member)

#define yref_ref_return(ret)
#define yref_return_get(expr, dst)
#define yref_return_member_get(expr, dst, member)

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
#ifndef YREF_CHECK
	tmp = yatomic_dec(&r->ref_count);
	assert(tmp >= 0);
	if (tmp == 0) {
		r->dtor(p);
		return true;
	}
	return false;
#else
	mtx_lock(&r->ref_mtx);
	struct yref *ref;
	HASH_FIND_PTR(r->referers, &pp, ref);
	assert(ref);
	assert(*ref->owner == p);
	if (ref->parent) {
		mtx_lock(&ref->parent->child_mtx);
		ylist_del(&ref->children);
		mtx_unlock(&ref->parent->child_mtx);
	}
	HASH_DEL(r->referers, ref);
	tmp = (--r->ref_count);
	mtx_unlock(&r->ref_mtx);
	assert(tmp >= 0);
	if (tmp == 0) {
		r->dtor(p);
		return true;
	}
	return false;
#endif
}

static inline void _yref_ref(void **pp, yref_t *r, yref_t *parent) {
	void *p = r->start;
	*pp = p;
#ifndef YREF_CHECK
	yatomic_inc(&r->ref_count);
#else
	struct yref *new_ref = talloc(1, struct yref);
	new_ref->owner = pp;
	new_ref->parent = parent;

	mtx_lock(&parent->child_mtx);
	ylist_add(&new_ref->children, &r->children);
	mtx_unlock(&parent->child_mtx);

	mtx_lock(&r->ref_mtx);
	++r->ref_count;
	HASH_ADD_PTR(r->referers, owner, new_ref);
	mtx_unlock(&r->ref_mtx);
#endif
}

static inline void yref_misuse_check(yref_t *r) {
	struct yref *tmp, *ref;
	HASH_ITER(hh, r->referers, ref, tmp) {
		assert(*ref->owner == r->start);
	}
}

static bool _yref_dfs(yref_t *r, int depth) {
	if (r->visited && r->visited < depth)
		return true;
	r->visited = depth;
	struct yref *ref;
	ylist_for_each_entry(ref, &r->children, children) {
		if (ref->info->visited > depth)
			continue;
		if (_yref_dfs(ref->info, depth+1))
			return true;
	}
	return false;
}

static void _yref_clear(yref_t *r) {
	r->visited = 0;
	struct yref *ref;
	ylist_for_each_entry(ref, &r->children, children) {
		if (!ref->info->visited)
			continue;
		_yref_clear(ref->info);
	}
}

static inline bool yref_circular_ref_check(yref_t *r){
	bool ret = _yref_dfs(r, 1);
	_yref_clear(r);
	return ret;
}
