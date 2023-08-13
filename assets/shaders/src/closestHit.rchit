#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : require

#include "globals.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
hitAttributeEXT vec2 hitAttributes;

layout(set = 2, binding = 1, scalar) buffer _InstanceInfo { InstanceData ref[]; };

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices { uint i[]; };

Tri getTriangle(uint primitiveIndex)
{
    Tri tri;
    const uint triangleIndex = primitiveIndex * 3;

    Indices indices = Indices(ref[gl_InstanceCustomIndexEXT].indexReference);
    Vertices vertices = Vertices(ref[gl_InstanceCustomIndexEXT].vertexReference);

    for(uint i = 0; i < 3; ++i)
    {
        const uint idx = indices.i[triangleIndex + i];
        tri.vertices[i] = vertices.v[idx];
    }

    tri.barycentrics = vec3(1.0 - hitAttributes.x - hitAttributes.y, hitAttributes.x, hitAttributes.y);
    tri.interpolatedUV = tri.vertices[0].uv * tri.barycentrics.x + tri.vertices[1].uv * tri.barycentrics.y + tri.vertices[2].uv * tri.barycentrics.z;
    tri.interpolatedNormal = normalize(tri.vertices[0].normal * tri.barycentrics.x + tri.vertices[1].normal * tri.barycentrics.y + tri.vertices[2].normal * tri.barycentrics.z);
    tri.interpolatedNormal = normalize(vec3(tri.interpolatedNormal * gl_WorldToObjectEXT));

    tri.dfd1 = tri.vertices[1].position - tri.vertices[0].position;
    tri.dfd2 = tri.vertices[2].position - tri.vertices[0].position;
    tri.uvd1 = tri.vertices[1].uv - tri.vertices[0].uv;
    tri.uvd2 = tri.vertices[2].uv - tri.vertices[0].uv;

    tri.materialId = ref[gl_InstanceCustomIndexEXT].materialId;

    return tri;
}

void main()
{
    payload.hit = true;
    payload.hitPoint = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    payload.triangle = getTriangle(gl_PrimitiveID);
}
