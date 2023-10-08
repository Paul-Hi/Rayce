struct Random
{
    static uint4 gState;

    static void init(uint4 seed)
    {
        gState = seed;
    }

    // http://www.jcgt.org/published/0009/03/02/
    static uint4 pcg4d()
    {
        gState = gState * 1664525u + 1013904223u;

        gState.x += gState.y * gState.w;
        gState.y += gState.z * gState.x;
        gState.z += gState.x * gState.y;
        gState.w += gState.y * gState.z;

        gState ^= gState >> 16u;

        gState.x += gState.y * gState.w;
        gState.y += gState.z * gState.x;
        gState.z += gState.x * gState.y;
        gState.w += gState.y * gState.z;

        return gState;
    }

    static float4 rand4()
    {
        const uint4 rInt = pcg4d();
        gState.w++;
        return float4(asfloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.y >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.z >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.w >> 9 | 0x3f800000) - 1.0
                      );
    }

    static float3 rand3()
    {
        uint4 rInt = pcg4d();
        gState.w++;
        return float3(asfloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.y >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.z >> 9 | 0x3f800000) - 1.0
                      );
    }

    static float2 rand2()
    {
        uint4 rInt = pcg4d();
        gState.w++;
        return float2(asfloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                      asfloat(rInt.y >> 9 | 0x3f800000) - 1.0
                      );
    }

    static float rand()
    {
        uint4 rInt = pcg4d();
        gState.w++;
        return asfloat(rInt.x >> 9 | 0x3f800000) - 1.0;
    }
};
