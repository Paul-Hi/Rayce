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

struct IntersectionInfo
{
    float2 uv;
    Material material;
    CoordinateSpace space;

    int lightId;
};

void getIntersectionInfo(const RayPayload payload, out IntersectionInfo iInfo)
{
    if(payload.hitKind == EHitKind::triangleMesh)
    {
        DeviceTriangle triangle = getTriangle(payload.primitiveId, payload.instanceCustomIndex, payload.hitAttributes, payload.worldToObject);
        iInfo.material = gMaterials[triangle.materialId];
        iInfo.uv = triangle.interpolatedUV;
        iInfo.lightId = triangle.lightId;
        iInfo.space = CoordinateSpace(iInfo.material.faceNormals == 1 ? triangle.geometryNormal : triangle.interpolatedNormal, iInfo.material.canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
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
            float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

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

        while(true)
        {
            traceRay(pathState.scatterRay, payload);

            if(payload.hitKind == EHitKind::miss)
            {
                // incorporate infinite lights?
                pathState.L += pathState.throughput * float3(0.01, 0.01, 0.05);
                break;
            }
            else
            {
                getIntersectionInfo(payload, iInfo);

                if(iInfo.lightId >= 0)
                {
                    const bool orientationValid = lightSampleOrientationValid(iInfo.lightId, pathState.scatterRay.origin, payload.hitPoint);
                    if(orientationValid)
                    {

                        if(depth == 0 || specularBounce)
                        {
                            pathState.L += pathState.throughput * gLights[iInfo.lightId].radiance;
                        }
                        else
                        {
                            const float lightPdf = pdfLight(iInfo.lightId, pathState.scatterRay.origin, payload.hitPoint);
                            const float miWeight = powerHeuristic(pdfBxDF, lightPdf);

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
                let materialInstance = material.getMaterialInstance(iInfo.uv);
                let adapter = material.getAdapter(iInfo.material.twoSided == 1);

                if(isNonSpecular(materialInstance.flags()))
                {
                    const Optional<LightSample> lightSample = sampleLights(payload.hitPoint, iInfo.lightId, iInfo.space, iInfo.material.twoSided == 1);
                    if(lightSample != none)
                    {
                        const float bsdfPdf = adapter.pdf(materialInstance, wo, lightSample.value.wi);
                        const float miWeight = powerHeuristic(lightSample.value.pdf, bsdfPdf);
                        const float3 f = adapter.f(materialInstance, wo, lightSample.value.wi) * abs(cosThetaTS(lightSample.value.wi));
                        pathState.L += pathState.throughput * miWeight * f * lightSample.value.radiance / lightSample.value.pdf;
                    }
                }

                const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wo);

                if(optionalSample == none)
                {
                    break;
                }

                const BxDFSample bxdfSample = optionalSample.value;

                pathState.throughput *= bxdfSample.f * abs(cosThetaTS(bxdfSample.wi)) / bxdfSample.pdf;
                pdfBxDF = bxdfSample.proportionalPDF ? adapter.pdf(materialInstance, wo, bxdfSample.wi) : bxdfSample.pdf;
                specularBounce = bxdfSample.isSpecular();
                anyNonSpecularBounces |= !specularBounce;

                if(bxdfSample.isTransmission())
                {
                    etaScale *= sqr(bxdfSample.eta);
                }

                // russian roulette
                const float3 rrBeta = etaScale * pathState.throughput;
                const float maxRRBeta = max(max(rrBeta.x, rrBeta.y), rrBeta.z);
                if (maxRRBeta < 1.0 && depth > russianRouletteStart)
                {
                    const float q = max(1.0 - maxRRBeta, 0.0);
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

struct BidirectionIntegrator : IIntegrator
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
            float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

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

        while(true)
        {
            traceRay(pathState.scatterRay, payload);

            if(payload.hitKind == EHitKind::miss)
            {
                // incorporate infinite lights?
                pathState.L += pathState.throughput * float3(0.01, 0.01, 0.05);
                break;
            }
            else
            {
                getIntersectionInfo(payload, iInfo);

                if(iInfo.lightId >= 0)
                {
                    const bool orientationValid = lightSampleOrientationValid(iInfo.lightId, pathState.scatterRay.origin, payload.hitPoint);
                    if(orientationValid)
                    {

                        if(depth == 0 || specularBounce)
                        {
                            pathState.L += pathState.throughput * gLights[iInfo.lightId].radiance;
                        }
                        else
                        {
                            const float lightPdf = pdfLight(iInfo.lightId, pathState.scatterRay.origin, payload.hitPoint);
                            const float miWeight = powerHeuristic(pdfBxDF, lightPdf);

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
                let materialInstance = material.getMaterialInstance(iInfo.uv);
                let adapter = material.getAdapter(iInfo.material.twoSided == 1);

                if(isNonSpecular(materialInstance.flags()))
                {
                    const Optional<LightSample> lightSample = sampleLights(payload.hitPoint, iInfo.lightId, iInfo.space, iInfo.material.twoSided == 1);
                    if(lightSample != none)
                    {
                        const float bsdfPdf = adapter.pdf(materialInstance, wo, lightSample.value.wi);
                        const float miWeight = powerHeuristic(lightSample.value.pdf, bsdfPdf);
                        const float3 f = adapter.f(materialInstance, wo, lightSample.value.wi) * abs(cosThetaTS(lightSample.value.wi));
                        pathState.L += pathState.throughput * miWeight * f * lightSample.value.radiance / lightSample.value.pdf;
                    }
                }

                const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wo);

                if(optionalSample == none)
                {
                    break;
                }

                const BxDFSample bxdfSample = optionalSample.value;

                pathState.throughput *= bxdfSample.f * abs(cosThetaTS(bxdfSample.wi)) / bxdfSample.pdf;
                pdfBxDF = bxdfSample.proportionalPDF ? adapter.pdf(materialInstance, wo, bxdfSample.wi) : bxdfSample.pdf;
                specularBounce = bxdfSample.isSpecular();
                anyNonSpecularBounces |= !specularBounce;

                if(bxdfSample.isTransmission())
                {
                    etaScale *= sqr(bxdfSample.eta);
                }

                // russian roulette
                const float3 rrBeta = etaScale * pathState.throughput;
                const float maxRRBeta = max(max(rrBeta.x, rrBeta.y), rrBeta.z);
                if (maxRRBeta < 1.0 && depth > russianRouletteStart)
                {
                    const float q = max(1.0 - maxRRBeta, 0.0);
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

        Ray scatterRay;

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
            float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

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
        pathState.scatterRay = createPrimaryRay(uv);

        RayPayload payload;
        IntersectionInfo iInfo;

        traceRay(pathState.scatterRay, payload);

        if(payload.hitKind == EHitKind::miss)
        {
            // incorporate infinite lights?
            return pathState;
        }
        else
        {
            getIntersectionInfo(payload, iInfo);

            if(iInfo.lightId >= 0)
            {
                const bool orientationValid = lightSampleOrientationValid(iInfo.lightId, pathState.scatterRay.origin, payload.hitPoint);
                if(orientationValid)
                {
                    pathState.L += gLights[iInfo.lightId].radiance;
                }
            }

            const float3 wo = iInfo.space.worldToTangentFrame(-pathState.scatterRay.direction);

            let material = iInfo.material.getMaterial();
            let materialInstance = material.getMaterialInstance(iInfo.uv);
            let adapter = material.getAdapter(iInfo.material.twoSided == 1);

            if(isNonSpecular(materialInstance.flags()))
            {
                const Optional<LightSample> lightSample = sampleLights(payload.hitPoint, iInfo.lightId, iInfo.space, iInfo.material.twoSided == 1);
                if(lightSample != none)
                {
                    const float bsdfPdf = adapter.pdf(materialInstance, wo, lightSample.value.wi);
                    const float miWeight = powerHeuristic(lightSample.value.pdf, bsdfPdf);
                    const float3 f = adapter.f(materialInstance, wo, lightSample.value.wi) * abs(cosThetaTS(lightSample.value.wi));
                    pathState.L += miWeight * f * lightSample.value.radiance / lightSample.value.pdf;
                }
            }

            const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wo);

            if(optionalSample == none)
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
            float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

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

        const Ray cameraRay = createPrimaryRay(uv);

        RayPayload payload;
        IntersectionInfo iInfo;

        traceRay(cameraRay, payload);

        if(payload.hitKind == EHitKind::miss)
        {
            // incorporate infinite lights?
            return pathState;
        }
        else
        {
            getIntersectionInfo(payload, iInfo);

            let material = iInfo.material.getMaterial();

            pathState.depth = (length(payload.hitPoint - cameraRay.origin) - gCamera.pbData.z) / (gCamera.pbData.w - gCamera.pbData.z);
            pathState.normal = iInfo.space.normal;

            if(iInfo.lightId >= 0)
            {
                const bool orientationValid = lightSampleOrientationValid(iInfo.lightId, cameraRay.origin, payload.hitPoint);
                if(orientationValid)
                {
                    pathState.emission = gLights[iInfo.lightId].radiance;
                }
            }

            const float3 wi = iInfo.space.worldToTangentFrame(-cameraRay.direction);

            let materialInstance = material.getMaterialInstance(iInfo.uv);
            let adapter = material.getAdapter(iInfo.material.twoSided == 1);

            const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wi);
            pathState.reflectance = optionalSample != none ? optionalSample.value.f : float3(0.0);
        }

        return pathState;
    }
};
