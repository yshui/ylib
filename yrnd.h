#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ydef.h"

struct yrnd_s128 {
	uint64_t s[2];
};
struct yrnd_s1024 {
	uint64_t s[16];
};

/* This is a xorshift RNG written in 2014 by Sebastiano Vigna (vigna@acm.org)
 * Period: 2^128-1, Speed: 1.10ns/64bit
 * Details: http://xorshift.di.unimi.it/
 */

static inline uint64_t
yrnd_xorshift128p(struct yrnd_s128 *state) {
	uint64_t s1 = state->s[0];
	const uint64_t s0 = state->s[1];
	state->s[0] = s0;
	s1 ^= s1<<23; // a
	state->s[1] = s1^s0^(s1>>17)^(s0>>26);
	return state->s[1]+s0; // b, c
}


struct _yrnd_xorshift1024s {
	struct yrnd_s1024 s;
	uint8_t p;
};

/* This is a xorshift RNG written in 2014 by Sebastiano Vigna (vigna@acm.org)
 * Period: 2^1024-1, Speed: 1.36ns/64bit
 * Details: http://xorshift.di.unimi.it/
 */

static inline
uint64_t yrnd_xorshift1024s(void *s) {
	struct _yrnd_xorshift1024s *_s =
		(struct _yrnd_xorshift1024s *)s;
	uint64_t *ss = _s->s.s;
	uint64_t s0 = ss[_s->p];
	_s->p = (_s->p+1)&15;

	uint64_t s1 = ss[_s->p];

	s1 ^= s1<<31; // a
	s1 ^= s1>>11; // b
	s0 ^= s0>>30; // c

	ss[_s->p] = s0^s1;
	return ss[_s->p]*1181783497276652981LL; 
}

static inline
void *yrnd_xorshift1024s_init(struct yrnd_s1024 *s) {
	struct _yrnd_xorshift1024s *tmp = talloc(1, struct _yrnd_xorshift1024s);
	memcpy(tmp->s.s, s->s, sizeof(s->s));
	return tmp;
}
