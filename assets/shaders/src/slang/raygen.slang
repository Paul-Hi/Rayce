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

    const uint objIdx = NonUniformResourceIndex(gInstanceData[instanceCustomIndex].objectIndex);

    const let ib = gIndices[objIdx];
    const let vb = gVertices[objIdx];

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
    float3 origin;
    float3 direction;
};

Ray createRay(const float2 screenUV)
{
    Ray ray;
    ray.origin = float3(0.0, 0.0, 0.0);
    const float4 imagePlane = mul(gCamera.inverseProjection, float4(screenUV.x, screenUV.y, 1.0, 1.0));
    ray.direction = normalize(imagePlane.xyz);

    if(gCamera.pbData.x > 0.0)
    {
        float2 pointOnLens = gCamera.pbData.x * mapToConcentricDisc(Random::rand2());

        float focalD = gCamera.pbData.y / ray.direction.z;
        float3 focusPoint = ray.origin - focalD * ray.direction;

        ray.origin = float3(pointOnLens, 0.0);
        ray.direction = normalize(focusPoint - ray.origin);
    }

    ray.origin = mul(gCamera.inverseView, float4(ray.origin, 1.0)).xyz;
    ray.direction = normalize(mul(gCamera.inverseView, float4(ray.direction, 0.0)).xyz);

    return ray;
}

float3 tracePaths(Ray ray)
{
    const int maxDepth = 16; // hopyfully never reached
    const int russianRouletteStart = 3;

    float3 color = float3(0.0);
    float3 throughput = float3(1.0);

    LightSample lightSample;
    lightSample.pdf = 1.0;
    BxDFSample bxdfSample;
    bxdfSample.pdf = 1.0;
    bxdfSample.dirac = false;
    CoordinateSpace space;
    space.normal = float3(0.0);

    for(uint depth = 0; depth < maxDepth; ++depth)
    {
        RayPayload payload;
        RayDesc rayDescriptor;
        rayDescriptor.Origin = ray.origin + space.normal * EPSILON;
        rayDescriptor.Direction = ray.direction;
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
            Material mat;
            if(payload.hitInfo.hitKind == EHitKind::triangleMesh)
            {
                DeviceTriangle triangle = getTriangle(payload.hitInfo.primitiveId, payload.hitInfo.instanceCustomIndex, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject, ray.origin, payload.hitPoint);
                mat = gMaterials[triangle.materialId];
                uv = triangle.interpolatedUV;
                lightId = triangle.lightId;
                space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
            }
            else if(payload.hitInfo.hitKind == EHitKind::proceduralSphere)
            {
                DeviceSphere sphere = getSphere(payload.hitPoint, payload.hitInfo.instanceCustomIndex, payload.hitInfo.primitiveId, payload.hitInfo.hitAttributes);
                mat = gMaterials[sphere.materialId];
                uv = float2(0.0); // FIXME: No Sphere UVs yet.
                space = CoordinateSpace(sphere.normal);
                lightId = sphere.lightId;
            }
            let material = mat.getMaterial();

            if(lightId >= 0)
            {

                // uses bxdfSample from last point
                float lightPdf = (depth == 0 || bxdfSample.dirac) ? 1.0 : pdfLight(lightId, ray.origin, payload.hitPoint);
                if(lightPdf > 0.0)
                {
                    float miWeight = powerHeuristic(bxdfSample.pdf, lightPdf);

                    color += throughput * miWeight * gLights[lightId].radiance;
                }
            }

            const float3 wo = space.worldToTangentFrame(-ray.direction);

            let bsdf = material.createBxDF(uv);
            if(!bsdf.isDirac())
            {
                lightSample = sampleLights(payload.hitPoint, lightId, space);
                if(lightSample.valid)
                {
                    float bsdfPdf = bsdf.pdf(wo, lightSample.wi);
                    float miWeight = powerHeuristic(lightSample.pdf, bsdfPdf);
                    color += lightSample.pdf > 0.0 ? throughput * miWeight * bsdf.evaluate(wo, lightSample.wi) * lightSample.radiance / lightSample.pdf : float3(0.0);
                }
            }

            bxdfSample = bsdf.sample(wo);

            float nDotL = cosThetaTS(bxdfSample.wi);

            throughput *= bxdfSample.pdf > 0.0 ? bxdfSample.reflectance * abs(nDotL) / bxdfSample.pdf : 0.0;

            // russian roulette
            if (depth >= russianRouletteStart)
            {
                float p = max(max(throughput.x, max(throughput.y, throughput.z)), EPSILON);
                if (Random::rand() > p)
                {
                    break;
                }

                throughput *= 1.0 / p;
            }

            ray.origin = payload.hitPoint;
            ray.direction = space.tangentToWorld(bxdfSample.wi);
            continue;
        }

        color += throughput * float3(0.02, 0.04, 0.08); // BG

        break;
    }

    return color;
}

[shader("raygeneration")]
void main()
{
    int2 pixelIndex = int2(DispatchRaysIndex().xy);
    float4 accumColor = gPushConstants.frame > 0 ? gAccumulationImage[pixelIndex] : float4(0.0, 0.0, 0.0, 0.0);
    accumColor.rgb *= accumColor.w;
    const float2 pixelCorner = float2(DispatchRaysIndex().xy);

    float3 color = float3(0.0);
    Random::init(uint4(DispatchRaysIndex().xy, gPushConstants.frame, 2907));

    // Anti Aliasing
    float2 uv = (pixelCorner + Random::rand()) / float2(DispatchRaysDimensions().xy - uint2(1));
    uv = uv * 2.0 - 1.0;

    // create ray origin and direction from camera data.
    Ray ray = createRay(uv);

    color += tracePaths(ray);

    accumColor.rgb += color;
    accumColor.w += 1.0;
    accumColor.rgb /= accumColor.w;

    gAccumulationImage[pixelIndex] = accumColor;
    const float4 resultColor = float4(linearTosRGB(toneMapACES(accumColor.rgb)), 1.0);
    gResultImage[pixelIndex] = resultColor;
}
