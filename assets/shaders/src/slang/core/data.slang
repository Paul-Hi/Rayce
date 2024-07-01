import core.globals;

[[vk::binding(TLAS_BINDING, RT_SET)]]
RaytracingAccelerationStructure gTLAS;
// layout(set = RT_SET, binding = TLAS_BINDING) uniform accelerationStructureEXT TLAS;

[[vk::binding(ACCUM_BINDING, RT_SET)]]
RWTexture2D<float4> gAccumulationImage;
// layout(set = RT_SET, binding = ACCUM_BINDING, rgba32f) uniform image2D accumulationImage;

[[vk::binding(RESULT_BINDING, RT_SET)]]
RWTexture2D<float4> gResultImage;
// layout(set = RT_SET, binding = RESULT_BINDING, rgba8) uniform image2D resultImage;

// layout(location = 0) rayPayloadEXT RayPayload pld;

[[vk::binding(CAMERA_BINDING, CAMERA_SET)]]
ConstantBuffer<CameraDataRT> gCamera;

// layout(set = CAMERA_SET, binding = CAMERA_BINDING, scalar) uniform CameraBufferRT
// {
//     mat4 inverseView;
//     mat4 inverseProjection;
// } camera;

[[vk::binding(TEXTURE_BINDING, MODEL_SET)]]
Sampler2D gTextures[];

// [[vk::binding(TEXTURE_BINDING,MODEL_SET)]]
// SamplerState gSamplers[];
// layout(set = MODEL_SET, binding = TEXTURE_BINDING) uniform sampler2D textures[];

[[vk::binding(INSTANCE_BINDING,MODEL_SET)]]
StructuredBuffer<InstanceData> gInstanceData;
// layout(set = MODEL_SET, binding = INSTANCE_BINDING, scalar) buffer _InstanceInfo { InstanceData ref[]; };

[[vk::binding(MATERIAL_BINDING,MODEL_SET)]]
StructuredBuffer<Material> gMaterials;
// layout(set = MODEL_SET, binding = MATERIAL_BINDING, scalar) buffer _MaterialInfo { Material materials[]; };

[[vk::binding(LIGHT_BINDING,MODEL_SET)]]
StructuredBuffer<Light> gLights;
// layout(set = MODEL_SET, binding = LIGHT_BINDING, scalar) buffer _LightInfo { Light lights[]; };

[[vk::binding(SPHERE_BINDING,MODEL_SET)]]
StructuredBuffer<Sphere> gSpheres;
// layout(set = MODEL_SET, binding = SPHERE_BINDING, scalar) buffer _SphereInfo { Sphere spheres[]; };

[[vk::binding(VERTEX_BINDING,INPUT_SET)]]
StructuredBuffer<Vertex> gVertices[];
// layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };p
[[vk::binding(INDEX_BINDING,INPUT_SET)]]
StructuredBuffer<uint> gIndices[];
// layout(buffer_reference, scalar) buffer Indices { uint i[]; };

struct PushConstants
{
    EIntegratorType integrator;
    int frame;
    int maxDepth;
    int lightCount;
    int environmentLightIdx;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> gPushConstants;


DeviceTriangle getTriangle(const uint primitiveIndex, const uint instanceCustomIndex, const float2 hitAttributes, const float4x3 worldToObject)
{
    DeviceTriangle tri;
    const uint triangleIndex = primitiveIndex * 3;

    const uint objIdx = NonUniformResourceIndex(gInstanceData[instanceCustomIndex].objectIndex);

    const let ib = gIndices[objIdx];
    const let vb = gVertices[objIdx];

    [ForceUnroll]
    for(uint i = 0; i < 3; ++i)
    {
        const uint idx = ib[triangleIndex + i];
        tri.vertices[i] = vb[idx];
    }

    tri.barycentrics = float3(1.0 - hitAttributes.x - hitAttributes.y, hitAttributes.x, hitAttributes.y);
    tri.interpolatedUV = tri.vertices[0].uv * tri.barycentrics.x + tri.vertices[1].uv * tri.barycentrics.y + tri.vertices[2].uv * tri.barycentrics.z;

    tri.interpolatedNormal = tri.vertices[0].normal * tri.barycentrics.x + tri.vertices[1].normal * tri.barycentrics.y + tri.vertices[2].normal * tri.barycentrics.z;
    tri.interpolatedNormal = normalize(mul(worldToObject, tri.interpolatedNormal).xyz);

    tri.dfd1 = tri.vertices[1].position - tri.vertices[0].position;
    tri.dfd2 = tri.vertices[2].position - tri.vertices[0].position;

    tri.uvd1 = tri.vertices[1].uv - tri.vertices[0].uv;
    tri.uvd2 = tri.vertices[2].uv - tri.vertices[0].uv;

    tri.geometryNormal = float3(
        tri.dfd1.y * tri.dfd2.z - tri.dfd1.z * tri.dfd2.y,
        tri.dfd1.z * tri.dfd2.x - tri.dfd1.x * tri.dfd2.z,
        tri.dfd1.x * tri.dfd2.y - tri.dfd1.y * tri.dfd2.x
    );
    tri.geometryNormal = normalize(mul(worldToObject, tri.geometryNormal).xyz);

    tri.materialId = gInstanceData[instanceCustomIndex].materialId;

    tri.lightId = gInstanceData[instanceCustomIndex].lightId;

    return tri;
}

DeviceSphere getSphere(const float3 hitPoint, const uint primitiveIndex, const uint instanceCustomIndex, const float4x3 worldToObject)
{
    DeviceSphere sphere;

    const int sphereId = gInstanceData[instanceCustomIndex + primitiveIndex].sphereId;

    const Sphere pSphere = gSpheres[sphereId];

    const float3 p = hitPoint - pSphere.center;
    sphere.normal = normalize(p);

    sphere.materialId = gInstanceData[instanceCustomIndex + primitiveIndex].materialId;

    sphere.lightId = gInstanceData[instanceCustomIndex + primitiveIndex].lightId;

    const float3 local = mul(worldToObject, hitPoint - pSphere.center).xyz;

    const float theta = acos(local.y / pSphere.radius);
    float phi = atan2(local.x, local.z);

    if (phi < 0.0)
        phi += TWO_PI;

    sphere.uv = float2(0.5 * phi, theta) * INV_PI;
    return sphere;
}
