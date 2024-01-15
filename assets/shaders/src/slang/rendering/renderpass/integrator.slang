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
            this.L = float3(0.0);
        }

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
            const float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

            const float focalD = gCamera.pbData.y / ray.direction.z;
            const float3 focusPoint = ray.origin - focalD * ray.direction;

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
        float etaScale = 1.0;

        RayPayload payload;
        uint depth = 0;

        while(true)
        {
            traceRay(pathState.scatterRay, payload);

            if(payload.hitKind == EHitKind::miss)
            {
                // incorporate infinite lights?
                break;
            }
            else
            {
                if(payload.lightId >= 0)
                {
                    const bool orientationValid = lightSampleOrientationValid(payload.lightId, pathState.scatterRay.origin, payload.hitPoint);
                    if(orientationValid)
                    {
                        if(depth == 0 || specularBounce)
                        {
                            pathState.L += pathState.throughput * gLights[payload.lightId].radiance;
                        }
                        else
                        {
                            const float lightPdf = pdfLight(payload.lightId, pathState.scatterRay.origin, payload.hitPoint);
                            const float miWeight = powerHeuristic(pdfBxDF, lightPdf);

                            pathState.L += pathState.throughput * miWeight * gLights[payload.lightId].radiance;
                        }
                    }
                }

                if (depth++ == maxDepth)
                {
                    break;
                }

                const float3 wo = payload.space.worldToTangentFrame(-pathState.scatterRay.direction);

                let materialData = gMaterials[payload.materialId];
                let material = materialData.getMaterial();
                let materialInstance = material.getMaterialInstance(payload.uv);
                let twoSided = materialData.twoSided == 1;
                let adapter = material.getAdapter(twoSided);

                if(isNonSpecular(materialInstance.flags()))
                {
                    const Optional<LightSample> lightSample = sampleLights(payload.hitPoint, payload.lightId, payload.space, twoSided);
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

                if(bxdfSample.isTransmission())
                {
                    etaScale *= sqr(bxdfSample.eta);
                }

                // russian roulette
                const float3 rrBeta = etaScale * pathState.throughput;
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

                pathState.scatterRay.origin = payload.hitPoint;
                pathState.scatterRay.direction = payload.space.tangentToWorld(bxdfSample.wi);
            }
        }

        return pathState;
    }
};

struct SimpleBidirectionalIntegrator : IIntegrator
{
    // 1 < s < max, t = 1
    struct PathState
    {
        __init()
        {
            this.scatterRay = Ray();
            this.throughput = float3(1.0);
            this.L = float3(0.0);
        }

        Ray scatterRay;

        float3 throughput;
        float3 L;
    };

    struct LightPathVertex
    {
        float3 position;
        float pdfFwd;
        float3 normal;
        float pdfBwd;
        float3 throughput;
        int lightId;
    };

    static const int maxLightPathLength = 7;

    struct LightPath
    {
        LightPathVertex vertices[maxLightPathLength];
        uint length;
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
            const float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

            const float focalD = gCamera.pbData.y / ray.direction.z;
            const float3 focusPoint = ray.origin - focalD * ray.direction;

            ray.origin = float3(pointOnLens, 0.0);
            ray.direction = normalize(focusPoint - ray.origin);
        }

        ray.origin = mul(gCamera.inverseView, float4(ray.origin, 1.0)).xyz;
        ray.direction = normalize(mul(gCamera.inverseView, float4(ray.direction, 0.0)).xyz);

        return ray;
    }

    Optional<VertexLightSample> initLightPath(out int lightIndex)
    {
        return sampleLightVertex(lightIndex);
    }

    float convertDensity(float pdf, const float3 nextPos, const float3 nextN, const float3 pos)
    {
        float3 w = nextPos - pos;
       float invDistSqrd = 1.0 / dot(w, w);
       pdf *= abs(dot(nextN, w * sqrt(invDistSqrd)));
        return pdf * invDistSqrd;
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
        const int russianRouletteStart = min(1, maxLightPathLength - 1);

        PathState pathState;
/*
        int lightIndex = -1;
        const Optional<VertexLightSample> optionalLightPathStart = initLightPath(lightIndex);
        if(optionalLightPathStart == none)
        {
            return pathState;
        }

        LightPath lightPath;

        lightPath.vertices[lightPath.length].lightId = lightIndex;
        lightPath.vertices[lightPath.length].normal = float3(0.0);
        lightPath.vertices[lightPath.length].pdfFwd = optionalLightPathStart.value.pdf;
        lightPath.vertices[lightPath.length].pdfBwd = 0.0;
        lightPath.vertices[lightPath.length].position = optionalLightPathStart.value.point;

        lightPath.vertices[lightPath.length].throughput = optionalLightPathStart.value.radiance;
        lightPath.length++;

        float3 wo = -optionalLightPathStart.value.wi;
        const CoordinateSpace lightSpace = CoordinateSpace(optionalLightPathStart.value.normal);
        pathState.scatterRay.origin = optionalLightPathStart.value.point;
        pathState.scatterRay.direction = lightSpace.tangentToWorld(-wo);
        pathState.throughput = optionalLightPathStart.value.radiance * abs(optionalLightPathStart.value.cosTheta) / (optionalLightPathStart.value.pdf * optionalLightPathStart.value.pdfDirection);

        float pdfFwd = optionalLightPathStart.value.pdf * optionalLightPathStart.value.pdfDirection;
        float pdfBwd = 0.0;
        bool specularBounce = false;
        float etaScale = 1.0;

        RayPayload payload;
        uint depth = 0;

        while(true)
        {
            traceRay(pathState.scatterRay, payload);

            if(payload.hitKind == EHitKind::miss)
            {
                break;
            }
            else
            {
                getIntersectionInfo(payload, payload.iInfo);

                if (depth++ == maxLightPathLength)
                {
                    break;
                }

                const float3 wo = payload.space.worldToTangentFrame(-pathState.scatterRay.direction);

                let material = gMaterials[payload.materialId].getMaterial();
                let materialInstance = material.getMaterialInstance(payload.uv);
                let adapter = material.getAdapter(gMaterials[payload.materialId].twoSided == 1);

                // compute vertex
                lightPath.vertices[lightPath.length].lightId = payload.lightId;
                lightPath.vertices[lightPath.length].normal = payload.space.normal;
                lightPath.vertices[lightPath.length].position = payload.hitPoint;
                lightPath.vertices[lightPath.length].pdfFwd = convertDensity(pdfFwd, lightPath.vertices[lightPath.length].position, lightPath.vertices[lightPath.length].normal, lightPath.vertices[lightPath.length - 1].position);
                lightPath.vertices[lightPath.length].pdfBwd = 0.0;
                lightPath.vertices[lightPath.length].throughput = pathState.throughput;

                const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wo);

                if(optionalSample == none)
                {
                    break;
                }

                const BxDFSample bxdfSample = optionalSample.value;

                pdfFwd = bxdfSample.pdf;
                pathState.throughput *= bxdfSample.f * abs(cosThetaTS(bxdfSample.wi)) / pdfFwd;
                specularBounce = bxdfSample.isSpecular();

                pdfBwd = adapter.pdf(materialInstance, bxdfSample.wi, wo);
                if(isSpecular(materialInstance.flags()))
                {
                    pdfFwd = pdfBwd = 0.0;
                }

                // FIXME: Correct Shading normal?

                lightPath.vertices[lightPath.length - 1].pdfBwd = convertDensity(pdfBwd, lightPath.vertices[lightPath.length].position, lightPath.vertices[lightPath.length].normal, lightPath.vertices[lightPath.length - 1].position);

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

                pathState.scatterRay.origin = payload.hitPoint;
                pathState.scatterRay.direction = payload.space.tangentToWorld(bxdfSample.wi);
                lightPath.length++;
            }
        }

        pathState.scatterRay = createPrimaryRay(uv);
        traceRay(pathState.scatterRay, payload);
        if(payload.hitKind == EHitKind::miss)
        {
            return pathState;
        }
        else
        {
            getIntersectionInfo(payload, payload.iInfo);

            const float3 wo = payload.space.worldToTangentFrame(-pathState.scatterRay.direction);

            let material = gMaterials[payload.materialId].getMaterial();
            let materialInstance = material.getMaterialInstance(payload.uv);
            let adapter = material.getAdapter(gMaterials[payload.materialId].twoSided == 1);

            if(!isNonSpecular(materialInstance.flags()))
            {
                return pathState;
            }

            for(int c = 0; c < lightPath.length; ++c)
            {
                // connect
                const float3 p = payload.hitPoint;
                const float3 normal = payload.space.normal;

            }
        }
*/
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
            const float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

            const float focalD = gCamera.pbData.y / ray.direction.z;
            const float3 focusPoint = ray.origin - focalD * ray.direction;

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

        traceRay(pathState.scatterRay, payload);

        if(payload.hitKind == EHitKind::miss)
        {
            // incorporate infinite lights?
            return pathState;
        }
        else
        {
            if(payload.lightId >= 0)
            {
                const bool orientationValid = lightSampleOrientationValid(payload.lightId, pathState.scatterRay.origin, payload.hitPoint);
                if(orientationValid)
                {
                    pathState.L += gLights[payload.lightId].radiance;
                }
            }

            const float3 wo = payload.space.worldToTangentFrame(-pathState.scatterRay.direction);

            let materialData = gMaterials[payload.materialId];
            let material = materialData.getMaterial();
            let materialInstance = material.getMaterialInstance(payload.uv);
            let twoSided = materialData.twoSided == 1;
            let adapter = material.getAdapter(twoSided);

            if(isNonSpecular(materialInstance.flags()))
            {
                const Optional<LightSample> lightSample = sampleLights(payload.hitPoint, payload.lightId, payload.space, twoSided);
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
            const float2 pointOnLens = gCamera.pbData.x * sampleUniformConcentricDisk(Random::rand2());

            const float focalD = gCamera.pbData.y / ray.direction.z;
            const float3 focusPoint = ray.origin - focalD * ray.direction;

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

        traceRay(cameraRay, payload);

        if(payload.hitKind == EHitKind::miss)
        {
            // incorporate infinite lights?
            return pathState;
        }
        else
        {
            let material = gMaterials[payload.materialId].getMaterial();

            pathState.depth = (length(payload.hitPoint - cameraRay.origin) - gCamera.pbData.z) / (gCamera.pbData.w - gCamera.pbData.z);
            pathState.normal = payload.space.normal;

            if(payload.lightId >= 0)
            {
                const bool orientationValid = lightSampleOrientationValid(payload.lightId, cameraRay.origin, payload.hitPoint);
                if(orientationValid)
                {
                    pathState.emission = gLights[payload.lightId].radiance;
                }
            }

            const float3 wi = payload.space.worldToTangentFrame(-cameraRay.direction);

            let materialInstance = material.getMaterialInstance(payload.uv);
            let adapter = material.getAdapter(gMaterials[payload.materialId].twoSided == 1);

            const Optional<BxDFSample> optionalSample = adapter.sample(materialInstance, wi);
            pathState.reflectance = optionalSample != none ? optionalSample.value.f : float3(0.0);
        }

        return pathState;
    }
};
