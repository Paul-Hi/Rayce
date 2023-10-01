import globals;
import utils;
import data;
import random;
import bsdf;
import sampling;

DeviceTriangle getTriangle(const uint primitiveIndex, const uint instanceCustomIndex, const float2 hitAttributes, const float4x3 worldToObject, const float3 viewPosition, const float3 hitPosition)
{
    DeviceTriangle tri;
    const uint triangleIndex = primitiveIndex * 3;

    const uint objIdx = gInstanceData.Load(instanceCustomIndex).objectIndex;

    for(uint i = 0; i < 3; ++i)
    {
        const uint idx = gIndices[objIdx].Load(triangleIndex + i);
        tri.vertices[i] = gVertices[objIdx].Load(idx);
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

    tri.materialId = gInstanceData.Load(instanceCustomIndex).materialId;

    tri.lightId = gInstanceData.Load(instanceCustomIndex).lightId;

    return tri;
}

DeviceSphere getSphere(const float3 hitPoint, const uint instanceCustomIndex, const uint primitiveIndex, const float2 hitAttributes)
{
    DeviceSphere sphere;

    int sphereId = gInstanceData.Load(instanceCustomIndex + primitiveIndex).sphereId;

    Sphere pShere = gSpheres.Load(sphereId);

    sphere.normal = normalize(hitPoint - pShere.center);

    sphere.materialId = gInstanceData.Load(instanceCustomIndex + primitiveIndex).materialId;

    sphere.lightId = gInstanceData.Load(instanceCustomIndex + primitiveIndex).lightId;

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
    BxDFSample bxdfSample;
    bxdfSample.pdf = 1.0;
    bxdfSample.dirac = true;
    CoordinateSpace space;
    space.normal = float3(0.0);

    for(uint depth = 0; depth < maxDepth; ++depth)
    {
        RayPayload payload;
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
            payload
        );

        if(payload.hit)
        {
            int lightId = -1;
            float2 uv;
            IMaterial material;
            if(payload.hitInfo.hitKind == EHitKind::triangleMesh)
            {
                return float3(0.0, 0.0, 1.0); // FIXME: Temporary
                DeviceTriangle triangle = getTriangle(payload.hitInfo.primitiveId, payload.hitInfo.instanceCustomIndex, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject, ray.origin.xyz, payload.hitPoint);
                material = gMaterials[triangle.materialId].getMaterial();
                uv = triangle.interpolatedUV;

                space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
            }
            else if(payload.hitInfo.hitKind == EHitKind::proceduralSphere)
            {
                return float3(0.0, 1.0, 0.0); // FIXME: Temporary
                DeviceSphere sphere = getSphere(payload.hitPoint, payload.hitInfo.instanceCustomIndex, payload.hitInfo.primitiveId, payload.hitInfo.hitAttributes);
                material = gMaterials[sphere.materialId].getMaterial();
                uv = float2(0.0); // FIXME: No Sphere UVs yet.
                space = CoordinateSpace(sphere.normal);
                lightId = sphere.lightId;
            }

            if(lightId >= 0)
            {
                // uses bxdfSample from last point
                float lightPdf = (depth == 0 || bxdfSample.dirac) ? 1.0 : pdfLight(lightId, ray.origin.xyz);
                float miWeight = powerHeuristic(bxdfSample.pdf, lightPdf);

                color += throughput * miWeight * gLights.Load(lightId).radiance;
            }

            const float3 wo = space.worldToTangentFrame(-ray.direction.xyz);

            if(!material.createBxDF(uv).isDirac())
            {
                lightSample = sampleLights(payload.hitPoint, lightId, space);
                if(lightSample.valid)
                {
                    float bsdfPdf = material.createBxDF(uv).pdf(wo, lightSample.wi);
                    float miWeight = powerHeuristic(lightSample.pdf, bsdfPdf);
                    color += throughput * miWeight * material.createBxDF(uv).evaluate(wo, lightSample.wi) * lightSample.radiance / lightSample.pdf;
                }
            }

            bxdfSample = material.createBxDF(uv).sample(wo);

            float nDotL = cosThetaTS(bxdfSample.wi);

            throughput *= bxdfSample.reflectance * abs(nDotL) / bxdfSample.pdf;

            // russian roulette
            if (depth >= russianRouletteStart)
            {
                float p = max(throughput.x, max(throughput.y, throughput.z));
                if (Random::rand() > p) {
                    break;
                }

                throughput *= 1.0 / p;
            }

            ray.origin.xyz = payload.hitPoint;
            ray.direction.xyz = space.tangentToWorld(bxdfSample.wi);
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
