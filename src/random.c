#include "random.h"

#include <immintrin.h>

uint64_t rdseed64()
{
    unsigned long long rnd;
    while (!_rdseed64_step(&rnd)) continue;
    return rnd;
}

uint64_t rol64(uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

uint64_t xoshiro256pp(struct xoshiro256pp_state *state)
{
    uint64_t *s = state->s;
    uint64_t const result = rol64(s[0] + s[3], 23) + s[0];
    uint64_t const t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rol64(s[3], 45);

    return result;
}

void xoshiro256pp_seed(struct xoshiro256pp_state *state)
{
    uint64_t *s = state->s;
    for (size_t i = 0; i < 4; i++)
        s[i] = rdseed64();
}
