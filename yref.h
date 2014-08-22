#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>


#include "ylist.h"
#include "yskiplist.h"
#include "ydef.h"
#include "compiler.h"

#define YREF_OWNER_RETURN ((void *)(-1))

typedef void (*yref_dtor)(void *);

struct yref_ref {
	//owner = address of pointer to the ref counted obj
	//parent = address of a ref counted obj that contains the owner
	void *owner, *parent;
	bool visited;
	struct ylist_head children;
	struct yskiplist_head owners;
};

struct yref {
	int ref_count;
	void *start;
	size_t len;
	yref_dtor dtor;
	struct ylist_head children;
	struct yskiplist_head owners;
};

#define yref_def_scope_out(type, member) void type##_scope_out_func(type **p) {\
	yref_unref_p(p, &(*p)->member); \
}

#define yref_def_ref(type, name) type *name Y_CLEANUP(type##_scope_out_func)

#define yref_ref(src, dst)
#define yref_ref_member(src, dst, member)

#define yref_move(src, dst)
#define yref_move_member(src, dst, member)

static inline int yref_owner_cmp(struct yskiplist_head *a, void *key){
	struct yref_ref *b = yskiplist_entry(a, struct yref_ref, owners);
	struct yref_ref *c = (struct yref_ref *)key;
	if (b->owner == c->owner)
		return 0;
	return b->owner < c->owner ? -1 : 1;
}

static inline void yref_unref(void *p, struct yref *r){
	struct yskiplist_head *tmp =
		yskiplist_extract_by_key(&r->owners, p, yref_owner_cmp);
	assert(tmp);
	struct yref_ref *ref = yskiplist_entry(tmp, struct yref_ref, owners);
	r->ref_count--;
	assert(r->ref_count >= 0);
	if (!r->ref_count)
		r->dtor(r->start);
}

static inline bool yref_circular_check(){
}
