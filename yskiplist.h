#pragma once

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ydef.h"

#ifndef MAX_HEIGHT
# define MAX_HEIGHT 32
#endif

#ifndef GET_RANDOM
# define GET_RANDOM (random())
#endif

struct yskiplist_head {
	struct yskiplist_head **next, **prev;
	int h;
};

#define yskiplist_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define yskiplist_empty(ptr) \
	((ptr)->next == NULL || (ptr)->next[0] == NULL)

static inline void yskiplist_init_head(struct yskiplist_head *h){
	h->h = MAX_HEIGHT;
	h->next = calloc(MAX_HEIGHT, sizeof(void *));
	h->prev = NULL;
}

static inline void yskiplist_deinit_head(struct yskiplist_head *h){
	h->h = 0;
	free(h->next);
	h->next = NULL;
}

static inline int yskiplist_gen_height(void){
	int r = GET_RANDOM;
	int h = 1;
	for(;r&1;r>>=1)
		h++;
	return h;
}

static inline void yskiplist_init_node(struct yskiplist_head *n){
	int newh = yskiplist_gen_height();
	n->next = talloc(newh, struct yskiplist_head *);
	n->prev = talloc(newh, struct yskiplist_head *);
	n->h = newh;
}

static inline void yskiplist_deinit_node(struct yskiplist_head *n){
	free(n->next);
	free(n->prev);
	n->next = n->prev = NULL;
	n->h = 0;
}

typedef int (*yskiplist_cmp)(struct yskiplist_head *a, void *key);

static int yskiplist_last_cmp(struct yskiplist_head *a, void *key){
	return a ? -1 : 1;
}

static inline void yskiplist_previous(struct yskiplist_head *head,
				      void *key, yskiplist_cmp cmp,
				      struct yskiplist_head **res){
	int h = head->h-1;
	struct yskiplist_head *n = head;
	while(1){
		while(h >= 0 &&
		     (n->next[h] == NULL || cmp(n->next[h], key) >= 0)){
			res[h] = n;
			h--;
		}
		if (h < 0)
			break;
		while(n->next[h] && cmp(n->next[h], key) < 0)
			n = n->next[h];
	}
}

static inline void
yskiplist_insert(struct yskiplist_head *h, struct yskiplist_head *n,
		      void *key, yskiplist_cmp cmp) {
	yskiplist_init_node(n);
	int i;
	struct yskiplist_head *hs[MAX_HEIGHT];
	yskiplist_previous(h, key, cmp, hs);
	for(i = 0; i < n->h; i++) {
		n->next[i] = hs[i]->next[i];
		hs[i]->next[i] = n;
		n->prev[i] = hs[i];
	}
	for(i = 0; i < n->h; i++) {
		if (!n->next[i])
			break;
		n->next[i]->prev[i] = n;
	}
}

//Find the smallest element that is greater than or equal to key.
static inline struct yskiplist_head *
yskiplist_find_ge(struct yskiplist_head *h, void *key, yskiplist_cmp cmp){
	int i;
	struct yskiplist_head *hs[MAX_HEIGHT];
	yskiplist_previous(h, key, cmp, hs);
	return hs[0]->next[0];
}

//Find the smallest element that is less than or equal to key.
static inline struct yskiplist_head *
yskiplist_find_le(struct yskiplist_head *h, void *key, yskiplist_cmp cmp){
	int i;
	struct yskiplist_head *hs[MAX_HEIGHT];
	yskiplist_previous(h, key, cmp, hs);
	if (hs[0]->next[0] && cmp(hs[0]->next[0], key) == 0)
		return hs[0]->next[0];
	return hs[0] == h ? NULL : hs[0];
}

static inline struct yskiplist_head *
yskiplist_extract_by_key(struct yskiplist_head *h, void *key,
			 yskiplist_cmp cmp){
	int i;
	struct yskiplist_head *hs[MAX_HEIGHT];
	yskiplist_previous(h, key, cmp, hs);
	struct yskiplist_head *node = hs[0]->next[0];
	if (!node || cmp(node, key) != 0)
		return NULL;
	for(i = 0; i < node->h; i++) {
		assert(hs[i] == node->prev[i]);
		hs[i]->next[i] = node->next[i];
		if (node->next[i])
			node->next[i]->prev[i] = node->prev[i];
	}
	yskiplist_deinit_node(node);
	return node;
}


static inline void
yskiplist_delete(struct yskiplist_head *h){
	int i;
	for(i = 0; i < h->h; i++) {
		h->prev[i]->next[i] = h->next[i];
		if (h->next[i])
			h->next[i]->prev[i] = h->prev[i];
	}
	yskiplist_deinit_node(h);
}

static inline void
yskiplist_clear(struct yskiplist_head *h,
		void (*freep)(struct yskiplist_head *)){
	struct yskiplist_head *hx = h->next[0];
	while(hx){
		struct yskiplist_head *tmp = hx->next[0];
		freep(hx);
		hx = tmp;
	}
}

