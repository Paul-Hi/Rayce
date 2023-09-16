#ifndef RANDOM_GLSL
#define RANDOM_GLSL

uvec4 rngState;

// http://www.jcgt.org/published/0009/03/02/
uvec4 pcg4d(uvec4 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y*v.w;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v.w += v.y*v.z;

    v ^= v >> 16u;

    v.x += v.y*v.w;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v.w += v.y*v.z;

    return v;
}

vec4 rand4()
{
    uvec4 rInt = pcg4d(rngState);
    rngState.w++;
    return vec4(uintBitsToFloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                uintBitsToFloat(rInt.y >> 9 | 0x3f800000) - 1.0,
                uintBitsToFloat(rInt.z >> 9 | 0x3f800000) - 1.0,
                uintBitsToFloat(rInt.w >> 9 | 0x3f800000) - 1.0
            );
}

vec3 rand3()
{
    uvec4 rInt = pcg4d(rngState);
    rngState.w++;
    return vec3(uintBitsToFloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                uintBitsToFloat(rInt.y >> 9 | 0x3f800000) - 1.0,
                uintBitsToFloat(rInt.z >> 9 | 0x3f800000) - 1.0
            );
}

vec2 rand2()
{
    uvec4 rInt = pcg4d(rngState);
    rngState.w++;
    return vec2(uintBitsToFloat(rInt.x >> 9 | 0x3f800000) - 1.0,
                uintBitsToFloat(rInt.y >> 9 | 0x3f800000) - 1.0
            );
}

float rand()
{
    uvec4 rInt = pcg4d(rngState);
    rngState.w++;
    return uintBitsToFloat(rInt.x >> 9 | 0x3f800000) - 1.0;
}

#endif  // RANDOM_GLSL