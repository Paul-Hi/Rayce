struct Random
{
    // https://prng.di.unimi.it/xoshiro128starstar.c
    // This is a Xoshiro128StarStar initialized with a SplitMix64 similarly executed to Nvidias Falcor
    static uint4 gState;

    static inline uint64_t splitMix64Rand(uint64_t state)
    {
        uint64_t z = (state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    static void init(uint2 seed)
    {
        uint64_t solitMix64State = (uint64_t(seed.x) << 32) | uint64_t(seed.y);
        uint64_t s0 = splitMix64Rand(solitMix64State);
        uint64_t s1 = splitMix64Rand(solitMix64State);
        gState = { uint(s0), uint(s0 >> 32), uint(s1), uint(s1 >> 32) };
    }

    static inline uint rotl(const uint x, int k)
    {
        return (x << k) | (x >> (32 - k));
    }

    static uint nextUint()
    {
        const uint result = rotl(gState.y * 5, 7) * 9;

        const uint t = gState.y << 9;

        gState.z ^= gState.x;
        gState.w ^= gState.y;
        gState.y ^= gState.z;
        gState.x ^= gState.w;

        gState.z ^= t;

        gState.w = rotl(gState.w, 11);

        return result;
    }

    static float rand()
    {
        // Upper 24 bits + divide by 2^24 to get a random number in [0,1).
        uint bits = nextUint();
        return (bits >> 8) * 0x1p-24;
    }

    static float2 rand2()
    {
        float2 result;
        result.x = rand();
        result.y = rand();
        return result;
    }
};
