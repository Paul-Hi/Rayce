#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "globals.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main()
{
    payload.hit = false;
}