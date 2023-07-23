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

    tri.barycentrics = vec3(1.0f - hitAttributes.x - hitAttributes.y, hitAttributes.x, hitAttributes.y);
    tri.interpolatedUV = tri.vertices[0].uv * tri.barycentrics.x + tri.vertices[1].uv * tri.barycentrics.y + tri.vertices[2].uv * tri.barycentrics.z;
    tri.materialId = ref[gl_InstanceCustomIndexEXT].materialId;

    return tri;
}

void main()
{
    payload.hitT = gl_HitTEXT;
    payload.triangle = getTriangle(gl_PrimitiveID);
}
