#ifndef RANDOM_GLSL
#define RANDOM_GLSL

uint wangHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

uint rngState;

uint xorshift32()
{
    rngState ^= (rngState << 13);
    rngState ^= (rngState >> 17);
    rngState ^= (rngState << 5);
    return rngState;
}

float rand()
{
    return uintBitsToFloat(xorshift32() >> 9 | 0x3f800000) - 1.0;
}

#endif  // RANDOM_GLSL