import globals;
import utils;
import data;
import random;
import bsdf;
import sampling;

interface IIntegrator
{
    Ray createPrimaryRay(const float2 uv);
    void traceRay(const Ray ray, inout RayPayload payload);
    associatedtype IntegratorPathState;
    IntegratorPathState estimate(const float2 uv);
};

struct PathIntegrator : IIntegrator
{
    struct PathState
    {
        __init()
        {
            this.bounces = 0;
            this.length = 0;
            this.scatterRay = Ray();
            this.throughput = float3(1.0);
            this.L = float3(0.0);
        }

        uint bounces;
        float length;

        Ray scatterRay;

        float3 throughput;
        float3 L;
    };

    typedef PathState IntegratorPathState;

    Ray createPrimaryRay(const float2 uv)
    {
        Ray ray;
        ray.origin = float3(0.0, 0.0, 0.0);
        const float4 imagePlane = mul(gCamera.inverseProjection, float4(uv.x, uv.y, 1.0, 1.0));
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

    void traceRay(const Ray ray, inout RayPayload payload)
    {
        RayDesc rayDescriptor;
        rayDescriptor.Origin = ray.offsetOrigin();
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
    }

    PathState estimate(const float2 uv)
    {
        const int maxDepth = gPushConstants.maxDepth; // hopyfully never reached
        const int russianRouletteStart = 3;

        PathState pathState;
        pathState.scatterRay = createPrimaryRay(uv);

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
            traceRay(pathState.scatterRay, payload);

            if(payload.hit)
            {
                int lightId = -1;
                float2 uv;
                Material mat;
                if(payload.hitInfo.hitKind == EHitKind::triangleMesh)
                {
                    DeviceTriangle triangle = getTriangle(payload.hitInfo.primitiveId, payload.hitInfo.instanceCustomIndex, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject, pathState.scatterRay.origin, payload.hitPoint);
                    mat = gMaterials[triangle.materialId];
                    uv = triangle.interpolatedUV;
                    lightId = triangle.lightId;
                    space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
                }
                else if(payload.hitInfo.hitKind == EHitKind::proceduralSphere)
                {
                    DeviceSphere sphere = getSphere(payload.hitPoint, payload.hitInfo.instanceCustomIndex, payload.hitInfo.primitiveId, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject);
                    mat = gMaterials[sphere.materialId];
                    uv = sphere.uv;
                    space = CoordinateSpace(sphere.normal, true);
                    lightId = sphere.lightId;
                }
                let material = mat.getMaterial();

                if(lightId >= 0)
                {
                    // uses bxdfSample from last point
                    float lightPdf = (depth == 0 || bxdfSample.dirac) ? 1.0 : pdfLight(lightId, pathState.scatterRay.origin, payload.hitPoint);
                    if(lightPdf > 0.0)
                    {
                        float miWeight = powerHeuristic(bxdfSample.pdf, lightPdf);

                        pathState.L += pathState.throughput * miWeight * gLights[lightId].radiance;
                    }
                }

                const float3 wo = space.worldToTangentFrame(-pathState.scatterRay.direction);

                let singleBsdf = material.createBxDF(uv);
                let bsdf = material.getAdapter(singleBsdf, mat.twoSided == 1);

                if(!bsdf.isDirac())
                {
                    lightSample = sampleLights(payload.hitPoint, lightId, space);
                    if(lightSample.valid)
                    {
                        float bsdfPdf = bsdf.pdf(wo, lightSample.wi);
                        float miWeight = powerHeuristic(lightSample.pdf, bsdfPdf);
                        pathState.L += lightSample.pdf > 0.0 ? pathState.throughput * miWeight * bsdf.evaluate(wo, lightSample.wi) * lightSample.radiance / lightSample.pdf : float3(0.0);
                    }
                }

                bxdfSample = bsdf.sample(wo);

                float nDotL = cosThetaTS(bxdfSample.wi);

                pathState.throughput *= bxdfSample.pdf > 0.0 ? bxdfSample.reflectance * abs(nDotL) / bxdfSample.pdf : 0.0;

                // russian roulette
                if (depth >= russianRouletteStart)
                {
                    float p = max(max(pathState.throughput.x, max(pathState.throughput.y, pathState.throughput.z)), EPSILON);
                    if (Random::rand() > p)
                    {
                        break;
                    }

                    pathState.throughput *= 1.0 / p;
                }

                pathState.bounces++;
                pathState.length += length(payload.hitPoint - pathState.scatterRay.origin);
                pathState.scatterRay.origin = payload.hitPoint;
                pathState.scatterRay.direction = space.tangentToWorld(bxdfSample.wi);

                continue;
            }

            break;
        }

        return pathState;
    }
};

struct DirectIntegrator : IIntegrator
{
    struct PathState
    {
        __init()
        {
            this.L = float3(0.0);
        }

        Ray shadowRay;

        float3 L;
    };

    typedef PathState IntegratorPathState;

    Ray createPrimaryRay(const float2 uv)
    {
        Ray ray;
        ray.origin = float3(0.0, 0.0, 0.0);
        const float4 imagePlane = mul(gCamera.inverseProjection, float4(uv.x, uv.y, 1.0, 1.0));
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

    void traceRay(const Ray ray, inout RayPayload payload)
    {
        RayDesc rayDescriptor;
        rayDescriptor.Origin = ray.offsetOrigin();
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
    }

    PathState estimate(const float2 uv)
    {
        PathState pathState;

        LightSample lightSample;
        lightSample.pdf = 1.0;
        BxDFSample bxdfSample;
        bxdfSample.pdf = 1.0;
        bxdfSample.dirac = false;
        CoordinateSpace space;
        space.normal = float3(0.0);

        RayPayload payload;
        Ray cameraRay = createPrimaryRay(uv);
        traceRay(cameraRay, payload);

        if(payload.hit)
        {
            int lightId = -1;
            float2 uv;
            Material mat;
            if(payload.hitInfo.hitKind == EHitKind::triangleMesh)
            {
                DeviceTriangle triangle = getTriangle(payload.hitInfo.primitiveId, payload.hitInfo.instanceCustomIndex, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject, cameraRay.origin, payload.hitPoint);
                mat = gMaterials[triangle.materialId];
                uv = triangle.interpolatedUV;
                lightId = triangle.lightId;
                space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
            }
            else if(payload.hitInfo.hitKind == EHitKind::proceduralSphere)
            {
                DeviceSphere sphere = getSphere(payload.hitPoint, payload.hitInfo.instanceCustomIndex, payload.hitInfo.primitiveId, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject);
                mat = gMaterials[sphere.materialId];
                uv = sphere.uv;
                space = CoordinateSpace(sphere.normal, true);
                lightId = sphere.lightId;
            }
            let material = mat.getMaterial();

            if(lightId >= 0)
            {
                pathState.L = gLights[lightId].radiance;

                return pathState;
            }

            const float3 wo = space.worldToTangentFrame(-cameraRay.direction);

            let singleBsdf = material.createBxDF(uv);
            let bsdf = material.getAdapter(singleBsdf, mat.twoSided == 1);

            if(!bsdf.isDirac())
            {
                lightSample = sampleLights(payload.hitPoint, lightId, space);
                if(lightSample.valid)
                {
                    float bsdfPdf = bsdf.pdf(wo, lightSample.wi);
                    float miWeight = powerHeuristic(lightSample.pdf, bsdfPdf);
                    pathState.L = lightSample.pdf > 0.0 ? miWeight * bsdf.evaluate(wo, lightSample.wi) * lightSample.radiance / lightSample.pdf : float3(0.0);
                    return pathState;
                }
            }
        }

        return pathState;
    }
};

struct DebugIntegrator : IIntegrator
{
    struct PathState
    {
        __init()
        {
            this.emission = float3(0.0);
            this.reflectance = float3(0.0);
            this.normal = float3(0.0);
            this.depth = 0.0;
        }

        float3 emission;
        float3 reflectance;
        float3 normal;
        float depth;
    };

    typedef PathState IntegratorPathState;

    Ray createPrimaryRay(const float2 uv)
    {
        Ray ray;
        ray.origin = float3(0.0, 0.0, 0.0);
        const float4 imagePlane = mul(gCamera.inverseProjection, float4(uv.x, uv.y, 1.0, 1.0));
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

    void traceRay(const Ray ray, inout RayPayload payload)
    {
        RayDesc rayDescriptor;
        rayDescriptor.Origin = ray.offsetOrigin();
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
    }

    PathState estimate(const float2 uv)
    {
        PathState pathState;

        CoordinateSpace space;
        space.normal = float3(0.0);

        RayPayload payload;
        Ray cameraRay = createPrimaryRay(uv);
        traceRay(cameraRay, payload);

        if(payload.hit)
        {
            int lightId = -1;
            float2 uv;
            Material mat;
            if(payload.hitInfo.hitKind == EHitKind::triangleMesh)
            {
                DeviceTriangle triangle = getTriangle(payload.hitInfo.primitiveId, payload.hitInfo.instanceCustomIndex, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject, cameraRay.origin, payload.hitPoint);
                mat = gMaterials[triangle.materialId];
                uv = triangle.interpolatedUV;
                lightId = triangle.lightId;
                space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
            }
            else if(payload.hitInfo.hitKind == EHitKind::proceduralSphere)
            {
                DeviceSphere sphere = getSphere(payload.hitPoint, payload.hitInfo.instanceCustomIndex, payload.hitInfo.primitiveId, payload.hitInfo.hitAttributes, payload.hitInfo.worldToObject);
                mat = gMaterials[sphere.materialId];
                uv = sphere.uv;
                space = CoordinateSpace(sphere.normal, true);
                lightId = sphere.lightId;
            }
            let material = mat.getMaterial();

            pathState.depth = (length(payload.hitPoint - cameraRay.origin) - gCamera.pbData.z) / (gCamera.pbData.w - gCamera.pbData.z);
            pathState.normal = space.normal;

            if(lightId >= 0)
            {
                pathState.emission = gLights[lightId].radiance;

                return pathState;
            }

            const float3 wo = space.worldToTangentFrame(-cameraRay.direction);

            let singleBsdf = material.createBxDF(uv);
            let bsdf = material.getAdapter(singleBsdf, mat.twoSided == 1);

            pathState.reflectance = bsdf.sample(wo).reflectance;
        }

        return pathState;
    }
};