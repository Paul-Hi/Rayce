#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "globals.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main()
{
    payload.hitT = INFINITY;
}