import globals;

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

struct CameraBufferRT
{
    float4x4 inverseView;
    float4x4 inverseProjection;
};

[[vk::binding(CAMERA_BINDING, CAMERA_SET)]]
ConstantBuffer<CameraBufferRT> gCamera;

// layout(set = CAMERA_SET, binding = CAMERA_BINDING, scalar) uniform CameraBufferRT
// {
//     mat4 inverseView;
//     mat4 inverseProjection;
// } camera;

[[vk::binding(TEXTURE_BINDING, MODEL_SET)]]
Texture2D gTextures[];

[[vk::binding(SAMPLER_BINDING,MODEL_SET)]]
SamplerState gSamplers[];
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

// FIXME: Is ConstBufferPointer stuff better here?
[[vk::binding(VERTEX_BINDING,RT_SET)]]
StructuredBuffer<Vertex> gVertices[];
// layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };p
[[vk::binding(INDEX_BINDING,RT_SET)]]
StructuredBuffer<uint> gIndices[];
// layout(buffer_reference, scalar) buffer Indices { uint i[]; };

struct PushConstants
{
    int frame;
    int lightCount;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> gPushConstants;
