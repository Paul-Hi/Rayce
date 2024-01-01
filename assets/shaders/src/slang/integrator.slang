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

struct IntersectionInfo
{
    float2 uv;
    Material material;
    CoordinateSpace space;

    int lightId;
};

// FIXME: Should use that, but inline produces invalid code
//[ForceInline]
void getIntersectionInfo(const RayPayload payload, out IntersectionInfo iInfo)
{
    if(payload.hitKind == EHitKind::triangleMesh)
    {
        DeviceTriangle triangle = getTriangle(payload.primitiveId, payload.instanceCustomIndex, payload.hitAttributes, payload.worldToObject);
        iInfo.material = gMaterials[triangle.materialId];
        iInfo.uv = triangle.interpolatedUV;
        iInfo.lightId = triangle.lightId;
        iInfo.space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
    }
    else if(payload.hitKind == EHitKind::proceduralSphere)
    {
        DeviceSphere sphere = getSphere(payload.hitPoint, payload.primitiveId, payload.instanceCustomIndex, payload.worldToObject);
        iInfo.material = gMaterials[sphere.materialId];
        iInfo.uv = sphere.uv;
        iInfo.lightId = sphere.lightId;
        iInfo.space = CoordinateSpace(sphere.normal, true);
    }
}

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
        const int russianRouletteStart = min(1, maxDepth - 1);

        PathState pathState;
        pathState.scatterRay = createPrimaryRay(uv);

        float pdfBxDF = 1.0;
        bool specularBounce = false;
        bool anyNonSpecularBounces = false;
        float etaScale = 1.0;

        RayPayload payload;
        IntersectionInfo iInfo;
        uint depth = 0;

        // FIXME Next: Rework everything to make it work more like pbrt:
        // That means:
        // - rework the bsdfs again
        //     - merge rough and non rough and compare to pbrt
        //     - add layered bsdf (maybe the falcor version here) and build the plastic from that to support mitsuba stuff

        while(true)
        {
            traceRay(pathState.scatterRay, payload);

            if(!payload.hit)
            {
                // incorporate infinite lights?
                break;
            }
            else
            {
                getIntersectionInfo(payload, iInfo); // FIXME

                if(iInfo.lightId >= 0)
                {
                    // uses bxdfSample from last point
                    bool orientationValid = lightSampleOrientationValid(iInfo.lightId, pathState.scatterRay.origin, payload.hitPoint);
                    if(orientationValid)
                    {

                        if(depth == 0 || specularBounce)
                        {
                            pathState.L += pathState.throughput * gLights[iInfo.lightId].radiance;
                        }
                        else
                        {
                            float lightPdf = pdfLight(iInfo.lightId, pathState.scatterRay.origin, payload.hitPoint);
                            float miWeight = powerHeuristic(pdfBxDF, lightPdf);

                            pathState.L += pathState.throughput * miWeight * gLights[iInfo.lightId].radiance;
                        }
                    }
                }

                if (depth++ == maxDepth)
                {
                    break;
                }

                const float3 wo = iInfo.space.worldToTangentFrame(-pathState.scatterRay.direction);

                let material = iInfo.material.getMaterial();
                let singleBsdf = material.createBxDF(iInfo.uv);
                let bsdf = material.getAdapter(singleBsdf, iInfo.material.twoSided == 1);

                if(isNonSpecular(bsdf.flags()))
                {
                    Optional<LightSample> lightSample = sampleLights(payload.hitPoint, iInfo.lightId, iInfo.space);
                    if(lightSample != none)
                    {
                        float bsdfPdf = bsdf.pdf(wo, lightSample.value.wi);
                        float miWeight = powerHeuristic(lightSample.value.pdf, bsdfPdf);
                        float3 f = bsdf.f(wo, lightSample.value.wi) * abs(cosThetaTS(lightSample.value.wi));
                        pathState.L += pathState.throughput * miWeight * f * lightSample.value.radiance / lightSample.value.pdf;
                    }
                }

                Optional<BxDFSample> optionalSample = bsdf.sample(wo);

                if(optionalSample == none)
                {
                    break;
                }

                BxDFSample bxdfSample = optionalSample.value;

                pathState.throughput *= bxdfSample.f * abs(cosThetaTS(bxdfSample.wi)) / bxdfSample.pdf;
                pdfBxDF = bxdfSample.proportionalPDF ? bsdf.pdf(wo, bxdfSample.wi) : bxdfSample.pdf;
                specularBounce = bxdfSample.isSpecular();
                anyNonSpecularBounces |= !specularBounce;

                if(bxdfSample.isTransmission())
                {
                    etaScale *= sqr(bxdfSample.eta);
                }

                // russian roulette
                float3 rrBeta = etaScale * pathState.throughput;
                float maxRRBeta = max(max(rrBeta.x, rrBeta.y), rrBeta.z);
                if (maxRRBeta < 1.0 && depth > russianRouletteStart)
                {
                    float q = max(1.0 - maxRRBeta, 0.0);
                    if (Random::rand() < q)
                    {
                        break;
                    }

                    pathState.throughput *= 1.0 / (1.0 - q);
                }

                pathState.bounces++;
                pathState.length += length(payload.hitPoint - pathState.scatterRay.origin);
                pathState.scatterRay.origin = payload.hitPoint;
                pathState.scatterRay.direction = iInfo.space.tangentToWorld(bxdfSample.wi);
            }
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
/*
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
            if(payload.hitKind == EHitKind::triangleMesh)
            {
                DeviceTriangle triangle = getTriangle(payload.primitiveId, payload.instanceCustomIndex, payload.hitAttributes, payload.worldToObject, cameraRay.origin, payload.hitPoint);
                mat = gMaterials[triangle.materialId];
                uv = triangle.interpolatedUV;
                lightId = triangle.lightId;
                space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
            }
            else if(payload.hitKind == EHitKind::proceduralSphere)
            {
                DeviceSphere sphere = getSphere(payload.hitPoint, payload.instanceCustomIndex, payload.primitiveId, payload.hitAttributes, payload.worldToObject);
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

            const float3 wi = space.worldToTangentFrame(-cameraRay.direction);

            let singleBsdf = material.createBxDF(uv);
            let bsdf = material.getAdapter(singleBsdf, mat.twoSided == 1);

            if(!bsdf.isDirac())
            {
                lightSample = sampleLights(payload.hitPoint, lightId, space);
                if(lightSample.valid)
                {
                    float bsdfPdf = bsdf.pdf(wi, lightSample.wo);
                    float miWeight = powerHeuristic(lightSample.pdf, bsdfPdf);
                    pathState.L = miWeight * bsdf.f(wi, lightSample.wo) * lightSample.radiance;
                    return pathState;
                }
            }
        }
*/
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
            if(payload.hitKind == EHitKind::triangleMesh)
            {
                DeviceTriangle triangle = getTriangle(payload.primitiveId, payload.instanceCustomIndex, payload.hitAttributes, payload.worldToObject);
                mat = gMaterials[triangle.materialId];
                uv = triangle.interpolatedUV;
                lightId = triangle.lightId;
                space = CoordinateSpace(triangle.interpolatedNormal, gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
            }
            else if(payload.hitKind == EHitKind::proceduralSphere)
            {
                DeviceSphere sphere = getSphere(payload.hitPoint, payload.primitiveId, payload.instanceCustomIndex, payload.worldToObject);
                mat = gMaterials[sphere.materialId];
                uv = sphere.uv;
                lightId = sphere.lightId;
                space = CoordinateSpace(sphere.normal, true);
            }
            let material = mat.getMaterial();

            pathState.depth = (length(payload.hitPoint - cameraRay.origin) - gCamera.pbData.z) / (gCamera.pbData.w - gCamera.pbData.z);
            pathState.normal = space.normal;

            if(lightId >= 0)
            {
                pathState.emission = gLights[lightId].radiance;

                return pathState;
            }

            const float3 wi = space.worldToTangentFrame(-cameraRay.direction);

            let singleBsdf = material.createBxDF(uv);
            let bsdf = material.getAdapter(singleBsdf, mat.twoSided == 1);

            Optional<BxDFSample> optionalSample = bsdf.sample(wi);
            pathState.reflectance = optionalSample != none ? optionalSample.value.f : float3(0.0);
        }

        return pathState;
    }
};