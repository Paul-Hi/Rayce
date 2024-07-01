import core.data;
import core.random;
__exported import rendering.renderingUtils;
import rendering.material.materialSystem;
import rendering.renderpass.sampling;

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
            this.scatterRay = Ray();
            this.throughput = float3(1.0);
            this.L          = float3(0.0);
        }

        Ray scatterRay;

        float3 throughput;
        float3 L;
    };

    typedef PathState IntegratorPathState;

    Ray createPrimaryRay(const float2 uv)
    {
        Ray ray;
        ray.origin              = float3(0.0, 0.0, 0.0);
        const float4 imagePlane = mul(gCamera.inverseProjection, float4(uv.x, uv.y, 1.0, 1.0));
        ray.direction           = normalize(imagePlane.xyz);

        if (gCamera.pbData.x > 0.0)
        {
            const float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

            const float focalD      = gCamera.pbData.y / ray.direction.z;
            const float3 focusPoint = ray.origin - focalD * ray.direction;

            ray.origin    = float3(pointOnLens, 0.0);
            ray.direction = normalize(focusPoint - ray.origin);
        }

        ray.origin    = mul(gCamera.inverseView, float4(ray.origin, 1.0)).xyz;
        ray.direction = normalize(mul(gCamera.inverseView, float4(ray.direction, 0.0)).xyz);

        return ray;
    }

    void traceRay(const Ray ray, inout RayPayload payload)
    {
        RayDesc rayDescriptor;
        rayDescriptor.Origin    = ray.offsetOrigin();
        rayDescriptor.Direction = ray.direction;
        rayDescriptor.TMin      = 0.001;
        rayDescriptor.TMax      = 1000.0;
        TraceRay(
            gTLAS,
            RAY_FLAG_FORCE_OPAQUE, // FIXME: | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xff,                  // cullMask
            0,                     // sbtRecordOffset
            0,                     // sbtRecordStride
            0,                     // missIndex
            rayDescriptor,
            payload);
    }

    PathState estimate(const float2 uv)
    {
        const int maxDepth             = gPushConstants.maxDepth; // hopyfully never reached
        const int russianRouletteStart = min(1, maxDepth - 1);

        PathState pathState;
        pathState.scatterRay = createPrimaryRay(uv);

        float pdfBxDF       = 1.0;
        bool specularBounce = false;
        float etaScale      = 1.0;

        RayPayload payload;
        uint depth = 0;

        while (true)
        {
            traceRay(pathState.scatterRay, payload);

            if (payload.hitKind == EHitKind::miss)
            {
                if (depth == 0 || specularBounce)
                {
                    pathState.L += pathState.throughput * getEnvironmentRadiance(pathState.scatterRay.direction);
                }
                else
                {
                    const float lightPdf = pdfLight(payload.lightId, pathState.scatterRay.origin, payload.hitPoint);
                    const float miWeight = powerHeuristic(pdfBxDF, lightPdf);

                    pathState.L += pathState.throughput * miWeight * getEnvironmentRadiance(pathState.scatterRay.direction);
                }
                break;
            }
            else
            {
                if (payload.lightId >= 0)
                {
                    const bool orientationValid = lightSampleOrientationValid(payload.lightId, pathState.scatterRay.origin, payload.hitPoint);
                    if (orientationValid)
                    {
                        if (depth == 0 || specularBounce)
                        {
                            pathState.L += pathState.throughput * gLights[payload.lightId].radiance * gLights[payload.lightId].scale;
                        }
                        else
                        {
                            const float lightPdf = pdfLight(payload.lightId, pathState.scatterRay.origin, payload.hitPoint);
                            const float miWeight = powerHeuristic(pdfBxDF, lightPdf);

                            pathState.L += pathState.throughput * miWeight * gLights[payload.lightId].radiance * gLights[payload.lightId].scale;
                        }
                    }
                }

                if (depth++ == maxDepth)
                {
                    break;
                }

                const float3 wo = payload.space.worldToTangentFrame(-pathState.scatterRay.direction);

                let materialData     = gMaterials[payload.materialId];
                let material         = materialData.getMaterial();
                let materialInstance = material.getMaterialInstance(payload.uv);
                let twoSided         = materialData.twoSided == 1;
                let adapter          = material.getAdapter(twoSided);

                if (isNonSpecular(materialInstance.flags()))
                {
                    const Optional<LightSample> lightSample = sampleLights(payload.hitPoint, payload.lightId, payload.space, twoSided);
                    if (lightSample != none)
                    {
                        const float bsdfPdf  = adapter.pdf(materialInstance, wo, lightSample.value.wi);
                        const float miWeight = powerHeuristic(lightSample.value.pdf, bsdfPdf);
                        const float3 f       = adapter.f(materialInstance, wo, lightSample.value.wi) * abs(cosThetaTS(lightSample.value.wi));
                        pathState.L += pathState.throughput * miWeight * f * lightSample.value.radiance / lightSample.value.pdf;
                    }
                }

                const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wo);

                if (optionalSample == none)
                {
                    break;
                }

                const BxDFSample bxdfSample = optionalSample.value;

                pathState.throughput *= bxdfSample.f * abs(cosThetaTS(bxdfSample.wi)) / bxdfSample.pdf;
                pdfBxDF        = bxdfSample.proportionalPDF ? adapter.pdf(materialInstance, wo, bxdfSample.wi) : bxdfSample.pdf;
                specularBounce = bxdfSample.isSpecular();

                if (bxdfSample.isTransmission())
                {
                    etaScale *= sqr(bxdfSample.eta);
                }

                // russian roulette
                const float3 rrBeta   = etaScale * pathState.throughput;
                const float maxRRBeta = averageLuminance(rrBeta);
                if (maxRRBeta < 1.0 && depth > russianRouletteStart)
                {
                    const float q = max(1.0 - maxRRBeta, 0.0);
                    if (Random::rand() < q)
                    {
                        break;
                    }

                    pathState.throughput *= 1.0 / (1.0 - q);
                }

                pathState.scatterRay.origin    = payload.hitPoint;
                pathState.scatterRay.direction = payload.space.tangentToWorld(bxdfSample.wi);
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

        Ray scatterRay;

        float3 L;
    };

    typedef PathState IntegratorPathState;

    Ray createPrimaryRay(const float2 uv)
    {
        Ray ray;
        ray.origin              = float3(0.0, 0.0, 0.0);
        const float4 imagePlane = mul(gCamera.inverseProjection, float4(uv.x, uv.y, 1.0, 1.0));
        ray.direction           = normalize(imagePlane.xyz);

        if (gCamera.pbData.x > 0.0)
        {
            const float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

            const float focalD      = gCamera.pbData.y / ray.direction.z;
            const float3 focusPoint = ray.origin - focalD * ray.direction;

            ray.origin    = float3(pointOnLens, 0.0);
            ray.direction = normalize(focusPoint - ray.origin);
        }

        ray.origin    = mul(gCamera.inverseView, float4(ray.origin, 1.0)).xyz;
        ray.direction = normalize(mul(gCamera.inverseView, float4(ray.direction, 0.0)).xyz);

        return ray;
    }

    void traceRay(const Ray ray, inout RayPayload payload)
    {
        RayDesc rayDescriptor;
        rayDescriptor.Origin    = ray.offsetOrigin();
        rayDescriptor.Direction = ray.direction;
        rayDescriptor.TMin      = 0.001;
        rayDescriptor.TMax      = 1000.0;
        TraceRay(
            gTLAS,
            RAY_FLAG_FORCE_OPAQUE, // FIXME: | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xff,                  // cullMask
            0,                     // sbtRecordOffset
            0,                     // sbtRecordStride
            0,                     // missIndex
            rayDescriptor,
            payload);
    }

    PathState estimate(const float2 uv)
    {
        PathState pathState;
        pathState.scatterRay = createPrimaryRay(uv);

        RayPayload payload;

        traceRay(pathState.scatterRay, payload);

        if (payload.hitKind == EHitKind::miss)
        {
            pathState.L += getEnvironmentRadiance(pathState.scatterRay.direction);
            return pathState;
        }
        else
        {
            if (payload.lightId >= 0)
            {
                const bool orientationValid = lightSampleOrientationValid(payload.lightId, pathState.scatterRay.origin, payload.hitPoint);
                if (orientationValid)
                {
                    pathState.L += gLights[payload.lightId].radiance;
                }
            }

            const float3 wo = payload.space.worldToTangentFrame(-pathState.scatterRay.direction);

            let materialData     = gMaterials[payload.materialId];
            let material         = materialData.getMaterial();
            let materialInstance = material.getMaterialInstance(payload.uv);
            let twoSided         = materialData.twoSided == 1;
            let adapter          = material.getAdapter(twoSided);

            if (isNonSpecular(materialInstance.flags()))
            {
                const Optional<LightSample> lightSample = sampleLights(payload.hitPoint, payload.lightId, payload.space, twoSided);
                if (lightSample != none)
                {
                    const float bsdfPdf  = adapter.pdf(materialInstance, wo, lightSample.value.wi);
                    const float miWeight = powerHeuristic(lightSample.value.pdf, bsdfPdf);
                    const float3 f       = adapter.f(materialInstance, wo, lightSample.value.wi) * abs(cosThetaTS(lightSample.value.wi));
                    pathState.L += miWeight * f * lightSample.value.radiance / lightSample.value.pdf;
                }
            }

            const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wo);

            if (optionalSample == none)
            {
                return pathState;
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
            this.emission    = float3(0.0);
            this.reflectance = float3(0.0);
            this.normal      = float3(0.0);
            this.depth       = 0.0;
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
        ray.origin              = float3(0.0, 0.0, 0.0);
        const float4 imagePlane = mul(gCamera.inverseProjection, float4(uv.x, uv.y, 1.0, 1.0));
        ray.direction           = normalize(imagePlane.xyz);

        if (gCamera.pbData.x > 0.0)
        {
            const float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

            const float focalD      = gCamera.pbData.y / ray.direction.z;
            const float3 focusPoint = ray.origin - focalD * ray.direction;

            ray.origin    = float3(pointOnLens, 0.0);
            ray.direction = normalize(focusPoint - ray.origin);
        }

        ray.origin    = mul(gCamera.inverseView, float4(ray.origin, 1.0)).xyz;
        ray.direction = normalize(mul(gCamera.inverseView, float4(ray.direction, 0.0)).xyz);

        return ray;
    }

    void traceRay(const Ray ray, inout RayPayload payload)
    {
        RayDesc rayDescriptor;
        rayDescriptor.Origin    = ray.offsetOrigin();
        rayDescriptor.Direction = ray.direction;
        rayDescriptor.TMin      = 0.001;
        rayDescriptor.TMax      = 1000.0;
        TraceRay(
            gTLAS,
            RAY_FLAG_FORCE_OPAQUE, // FIXME: | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xff,                  // cullMask
            0,                     // sbtRecordOffset
            0,                     // sbtRecordStride
            0,                     // missIndex
            rayDescriptor,
            payload);
    }

    PathState estimate(const float2 uv)
    {

        PathState pathState;

        const Ray cameraRay = createPrimaryRay(uv);

        RayPayload payload;

        traceRay(cameraRay, payload);

        if (payload.hitKind == EHitKind::miss)
        {
            // incorporate infinite lights?
            return pathState;
        }
        else
        {
            let material = gMaterials[payload.materialId].getMaterial();

            pathState.depth  = (length(payload.hitPoint - cameraRay.origin) - gCamera.pbData.z) / (gCamera.pbData.w - gCamera.pbData.z);
            pathState.normal = payload.space.normal;

            if (payload.lightId >= 0)
            {
                const bool orientationValid = lightSampleOrientationValid(payload.lightId, cameraRay.origin, payload.hitPoint);
                if (orientationValid)
                {
                    pathState.emission = gLights[payload.lightId].radiance;
                }
            }

            const float3 wi = payload.space.worldToTangentFrame(-cameraRay.direction);

            let materialInstance = material.getMaterialInstance(payload.uv);
            let adapter          = material.getAdapter(gMaterials[payload.materialId].twoSided == 1);

            const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wi);
            pathState.reflectance                     = optionalSample != none ? optionalSample.value.f : float3(0.0);
        }

        return pathState;
    }
};
