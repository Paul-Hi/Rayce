struct Random
{
    static uint4 state;

    static void init(uint4 seed)
    {
        state = seed;
    }

    // http://www.jcgt.org/published/0009/03/02/
    static uint4 pcg4d()
    {
        state = state * 1664525u + 1013904223u;

        state.x += state.y * state.w;
        state.y += state.z * state.x;
        state.z += state.x * state.y;
        state.w += state.y * state.z;

        state ^= state >> 16u;

        state.x += state.y * state.w;
        state.y += state.z * state.x;
        state.z += state.x * state.y;
        state.w += state.y * state.z;

        return state;
    }

    static float4 rand4()
    {
        const uint4 rInt = pcg4d();
        state.w++;
        return float4(asfloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.y >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.z >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.w >> 9 | 0x3f800000) - 1.0
                      );
    }

    static float3 rand3()
    {
        uint4 rInt = pcg4d();
        state.w++;
        return float3(asfloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.y >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.z >> 9 | 0x3f800000) - 1.0
                      );
    }

    static float2 rand2()
    {
        uint4 rInt = pcg4d();
        state.w++;
        return float2(asfloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.y >> 9 | 0x3f800000) - 1.0
                      );
    }

    static float rand()
    {
        uint4 rInt = pcg4d();
        state.w++;
        return asfloat(rInt.x >> 9 | 0x3f800000) - 1.0;
    }
};
