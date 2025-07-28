#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

struct xoshiro256pp_state {
    uint64_t s[4];
};

uint64_t rdseed64();

uint64_t rol64(uint64_t x, int k);

uint64_t xoshiro256pp(struct xoshiro256pp_state *state);

void xoshiro256pp_seed(struct xoshiro256pp_state *state);

#endif
