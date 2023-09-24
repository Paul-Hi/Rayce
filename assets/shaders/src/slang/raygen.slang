import globals;
import utils;
import random;
import bsdf;
import sampling;

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
SamplerState gDefaultSampler;
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
// layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
[[vk::binding(INDEX_BINDING,RT_SET)]]
StructuredBuffer<uint> gIndices[];
// layout(buffer_reference, scalar) buffer Indices { uint i[]; };

struct PushConstants
{
    int frame;
    int lightCount;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> gPushConstants;

DeviceTriangle getTriangle(const uint primitiveIndex, const uint instanceCustomIndex, const float2 hitAttributes, const float4x3 worldToObject, const float3 viewPosition, const float3 hitPosition)
{
    DeviceTriangle tri;
    const uint triangleIndex = primitiveIndex * 3;

    const StructuredBuffer<uint> indices = gIndices[gInstanceData[instanceCustomIndex].objectIndex];
    const StructuredBuffer<Vertex> vertices = gVertices[gInstanceData[instanceCustomIndex].objectIndex];

    for(uint i = 0; i < 3; ++i)
    {
        const uint idx = indices[triangleIndex + i];
        tri.vertices[i] = vertices[idx];
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

DeviceSphere getSphere(const float3 hitPoint, const uint instanceCustomIndex, const uint primitiveIndex, const float2 hitAttributes)
{
    DeviceSphere sphere;

    int sphereId = gInstanceData[instanceCustomIndex + primitiveIndex].sphereId;

    Sphere pShere = gSpheres[sphereId];

    sphere.normal = normalize(hitPoint - pShere.center);

    sphere.materialId = gInstanceData[instanceCustomIndex + primitiveIndex].materialId;

    sphere.lightId = gInstanceData[instanceCustomIndex + primitiveIndex].lightId;

    return sphere;
}

struct Ray
{
    float4 origin;
    float4 direction;
};

Ray createRay(const float2 screenUV)
{
    Ray ray;
    ray.origin = mul(gCamera.inverseView, float4(0.0, 0.0, 0.0, 1.0));
    float4 target = mul(gCamera.inverseProjection, float4(screenUV.x, screenUV.y, 1.0, 1.0));
    ray.direction = normalize(mul(gCamera.inverseView, float4(normalize(target.xyz), 0.0)));

    return ray;
}

float3 tracePaths(in float2 uv)
{
    const int maxDepth = 16; // hopyfully never reached
    const int russianRouletteStart = 3;

    float3 color = float3(0.0);
    float3 throughput = float3(1.0);

    // create ray origin and direction from camera data.
    Ray ray = createRay(uv);

    LightSample lightSample;
    lightSample.pdf = 1.0;
    BxDFSample bsdfSample;
    bsdfSample.pdf = 1.0;
    CoordinateSpace space;
    space.normal = float3(0.0);

    for(uint depth = 0; depth < maxDepth; ++depth)
    {
        RayPayload pld;
        RayDesc rayDescriptor;
        rayDescriptor.Origin = ray.origin.xyz + space.normal * EPSILON;
        rayDescriptor.Direction = ray.direction.xyz;
        rayDescriptor.TMin = 0.001;
        rayDescriptor.TMax = 1000.0;
        TraceRay(
            gTLAS,
            RAY_FLAG_FORCE_OPAQUE, // FIXME: | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xff,   // cullMask
            0,      // sbtRecordOffset
            0,      // sbtRecordStride
            0,      // missIndex
            rayDescriptor,
            pld
        );

        if(pld.hit)
        {
            int lightId = -1;
            Material material;
            if(pld.hitInfo.hitKind == EHitKind::triangleMesh)
            {
                DeviceTriangle triangle = getTriangle(pld.hitInfo.primitiveId, pld.hitInfo.instanceCustomIndex, pld.hitInfo.hitAttributes, pld.hitInfo.worldToObject, origin.xyz, pld.hitPoint);
                material = materials[triangle.materialId];
                populateSurfaceStateTriangle(material, triangle);
            }
            else if(pld.hitInfo.hitKind == EHitKind::proceduralSphere)
            {
                DeviceSphere sphere = getSphere(pld.hitPoint, pld.hitInfo.instanceCustomIndex, pld.hitInfo.primitiveId, pld.hitInfo.hitAttributes);
                material = materials[sphere.materialId];
                populateSurfaceStateSphere(material, sphere);
                lightId = sphere.lightId;
            }

            if(lightId >= 0)
            {
                float lightPdf = (depth == 0 || bsdfSample.dirac) ? 1.0 : pdfLight(lightId, origin.xyz);
                float miWeight = powerHeuristic(bsdfSample.pdf, lightPdf);

                color += throughput * miWeight * lights[lightId].radiance;
            }

            surfaceState.wo = worldToTangentFrame(-direction.xyz, surfaceState.tangent, surfaceState.bitangent, surfaceState.normal);

            if(!hasDiracPdf(material) && sampleLights(pld.hitPoint, lightId, lightSample))
            {
                float bsdfPdf = pdfBSDF(material);
                float miWeight = powerHeuristic(lightSample.pdf, bsdfPdf);
                color += throughput * miWeight * evaluateBSDF(material) * lightSample.radiance / lightSample.pdf;
            }

            if(!sampleBSDF(material, bsdfSample))
            {
                break;
            }

            float nDotL = cosThetaTS(surfaceState.wi);

            throughput *= bsdfSample.reflectance * abs(nDotL) / bsdfSample.pdf;

            // russian roulette
            if (depth >= russianRouletteStart)
            {
                float p = max(throughput.x, max(throughput.y, throughput.z));
                if (rand() > p) {
                    break;
                }

                throughput *= 1.0 / p;
            }

            origin.xyz = pld.hitPoint;
            direction.xyz = tangentToWorld(surfaceState.wi, surfaceState.tangent, surfaceState.bitangent, surfaceState.normal);
            continue;
        }

        color += throughput * float3(0.02, 0.04, 0.08);

        break;
    }

    return color;
}

[shader("raygeneration")]
void main()
{
    int2 pixelIndex = int2(DispatchRaysIndex().xy);
    float4 accumColor = gPushConstants.frame > 0 ? gAccumulationImage.Load(pixelIndex) : float4(0.0, 0.0, 0.0, 0.0);
    accumColor.rgb *= accumColor.w;
    const float2 pixelCorner = float2(DispatchRaysIndex().xy);

    float3 color = float3(0.0);
    Random::init(uint4(DispatchRaysIndex().xy, gPushConstants.frame, 2907));

    float2 uv = (pixelCorner + Random::rand2()) / float2(DispatchRaysDimensions().xy);
    uv = uv * 2.0 - 1.0;

    color += tracePaths(uv);

    accumColor.rgb += color;
    accumColor.w += 1.0;
    accumColor.rgb /= accumColor.w;

    gAccumulationImage[pixelIndex] = accumColor;
    const float4 resultColor = float4(linearTosRGB(toneMapACES(accumColor.rgb)), 1.0);
    gResultImage[pixelIndex] = resultColor;
}
