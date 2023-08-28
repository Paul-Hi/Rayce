#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : require

#include "globals.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
hitAttributeEXT vec2 hitAttributes;

void main()
{
    payload.hit = true;
    payload.hitPoint = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    payload.hitInfo.hitKind = hitKindTriangleMesh;
    payload.hitInfo.primitiveId = gl_PrimitiveID;
    payload.hitInfo.instanceCustomIndex = gl_InstanceCustomIndexEXT;
    payload.hitInfo.hitAttributes = hitAttributes;
    payload.hitInfo.worldToObject = gl_WorldToObjectEXT;
    payload.hitInfo.objectToWorld = gl_ObjectToWorldEXT;
}
