import rendering.renderingUtils;
import core.data;
import core.random;

static bool allowIncompletePDF = false;

struct GeometrySample
{
    float3 point;
    float pdf;
    float3 normal;
};

bool finalizeSolidAngleSampling(inout float pdf, const float3 normal, const float3 point, const float3 samplePoint)
{
    const float3 dir     = point - samplePoint;
    const float distSqrd = dot(dir, dir);
    const float cosTheta = dot(normal, dir / safe_sqrt(distSqrd));
    if (cosTheta <= 0.0)
    {
        return false;
    }
    pdf *= distSqrd / cosTheta;
    return true;
}

interface ISampler
{
    static GeometrySample sampleSolidAngle(const float3 point, const Light lightData);
    static float samplePdf(const float3 point, const Light lightData, const float3 samplePoint = float3(0.0));

    static bool sampleOrientationValid(const float3 point, const Light lightData, const float3 samplePoint = float3(0.0));

    static GeometrySample sampleArea(const Light lightData);
};

struct SphereSampler : ISampler
{
    static GeometrySample sampleUniform(const float3 center, const float radius, const float surfaceArea)
    {
        GeometrySample sample;
        const float2 uv        = Random::rand2();
        const float z          = 1.0 - 2.0 * uv.x;
        const float r          = safe_sqrt(1.0 - z * z);
        const float phi        = TWO_PI * uv.y;
        const float3 unitPoint = float3(r * cos(phi), r * sin(phi), z);
        sample.point           = center + unitPoint * radius;
        sample.pdf             = 1.0 / surfaceArea;
        sample.normal          = normalize(unitPoint);
        return sample;
    }

    static GeometrySample sampleSolidAngle(const float3 point, const Light lightData)
    {
        GeometrySample sample;
        float3 wDir      = lightData.wCenter - point;
        const float dist = length(wDir);
        wDir             = normalize(wDir);
        let space        = CoordinateSpace(wDir);

        const float radius = (mul(lightData.lightToWorld, float4(1.0, 0.0, 0.0, 1.0)).xyz - lightData.wCenter).x;

        if (dist <= radius) // inside
        {
            return SphereSampler::sampleUniform(lightData.wCenter, radius, lightData.surfaceArea);
        }

        const float distSqrd   = sqr(dist);
        const float radiusSqrd = sqr(radius);
        // max cone angles
        const float sinThetaMaxSqrd = radiusSqrd / distSqrd;
        const float cosThetaMax     = safe_sqrt(1.0 - sinThetaMaxSqrd);
        sample.pdf                  = INV_TWO_PI / (1.0 - cosThetaMax);
        // random in cone
        const float2 uv          = Random::rand2();
        const float cosTheta     = (1.0 - uv.x) + uv.x * cosThetaMax;
        const float sinThetaSqrd = max(0.0, 1.0 - sqr(cosTheta));
        const float phi          = TWO_PI * uv.y;
        // project to sphere surface
        const float cosAlpha = dist / radius * sinThetaSqrd + cosTheta * safe_sqrt(1.0 - distSqrd / radiusSqrd * sinThetaSqrd);
        const float sinAlpha = safe_sqrt(1.0 - sqr(cosAlpha));
        float3 samplePoint   = float3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
        samplePoint          = space.flipped().tangentToWorld(samplePoint);

        sample.point  = lightData.wCenter + samplePoint * radius;
        sample.normal = normalize(samplePoint);

        return sample;
    }

    static float samplePdf(const float3 point, const Light lightData, const float3 samplePoint)
    {
        const float3 wDir = lightData.wCenter - point;
        const float dist  = length(wDir);

        const float radius = (mul(lightData.lightToWorld, float4(1.0, 0.0, 0.0, 1.0)).xyz - lightData.wCenter).x;

        const bool inside = (dist <= radius);

        if (inside)
        {
            return 1.0 / lightData.surfaceArea;
        }

        const float sinThetaMaxSqrd = sqr(radius) / sqr(dist);
        const float cosThetaMax     = safe_sqrt(1.0 - sinThetaMaxSqrd);

        return INV_TWO_PI / (1.0 - cosThetaMax);
    }

    static bool sampleOrientationValid(const float3 point, const Light lightData, const float3 samplePoint)
    {
        const float3 wDir = lightData.wCenter - point;
        const float dist  = length(wDir);

        const float radius = (mul(lightData.lightToWorld, float4(1.0, 0.0, 0.0, 1.0)).xyz - lightData.wCenter).x;

        return (dist > radius);
    }

    static GeometrySample sampleArea(const Light lightData)
    {
        GeometrySample sample;

        const float radius = (mul(lightData.lightToWorld, float4(1.0, 0.0, 0.0, 1.0)).xyz - lightData.wCenter).x;

        return SphereSampler::sampleUniform(lightData.wCenter, radius, lightData.surfaceArea);
    }
}

struct RectangleSampler : ISampler
{
    static GeometrySample sampleSolidAngle(const float3 point, const Light lightData)
    {
        GeometrySample sample;

        // sample in -1, 1
        const float2 uv = Random::rand2() * 2.0 - 1.0;
        sample.point    = mul(lightData.lightToWorld, float4(uv.x, uv.y, 0.0, 1.0)).xyz;

        sample.pdf = 1.0 / lightData.surfaceArea;

        const float3 normal = normalize(mul(lightData.lightToWorld, float4(0.0, 0.0, 1.0, 0.0)).xyz);
        if (!finalizeSolidAngleSampling(sample.pdf, normal, point, sample.point))
        {
            sample.pdf = 0.0;
        }
        sample.normal = normal;

        return sample;
    }

    static float samplePdf(const float3 point, const Light lightData, const float3 samplePoint)
    {
        float pdf = 1.0 / lightData.surfaceArea;

        const float3 normal = normalize(mul(lightData.lightToWorld, float4(0.0, 0.0, 1.0, 0.0)).xyz);
        if (!finalizeSolidAngleSampling(pdf, normal, point, samplePoint))
        {
            return 0.0;
        }

        return pdf;
    }

    static bool sampleOrientationValid(const float3 point, const Light lightData, const float3 samplePoint)
    {
        const float3 w      = point - samplePoint;
        const float3 normal = normalize(mul(lightData.lightToWorld, float4(0.0, 0.0, 1.0, 0.0)).xyz);

        return dot(w, normal) > 0.0;
    }

    static GeometrySample sampleArea(const Light lightData)
    {
        GeometrySample sample;
        // sample in -1, 1
        const float2 uv = Random::rand2() * 2.0 - 1.0;
        sample.point    = mul(lightData.lightToWorld, float4(uv.x, uv.y, 0.0, 1.0)).xyz;

        sample.pdf = 1.0 / lightData.surfaceArea;

        const float3 normal = normalize(mul(lightData.lightToWorld, float4(0.0, 0.0, 1.0, 0.0)).xyz);
        sample.normal       = normal;

        return sample;
    }
}

float pdfLight(const int lightId, const float3 surfacePoint, const float3 samplePoint)
{
    const float pdf   = 1.0 / gPushConstants.lightCount;
    const Light light = gLights[lightId];

    if (light.type == ELightType::analyticSphere)
    {
        return SphereSampler::samplePdf(surfacePoint, light) * pdf;
    }
    else if (light.type == ELightType::analyticRectangle)
    {
        return RectangleSampler::samplePdf(surfacePoint, light, samplePoint) * pdf;
    }
    else if (light.type == ELightType::constant)
    {
        if (allowIncompletePDF)
        {
            return 0.0;
        }
        return INV_FOUR_PI;
    }

    return 0.0;
}

bool lightSampleOrientationValid(const int lightId, const float3 surfacePoint, const float3 samplePoint)
{
    const Light light = gLights[lightId];

    if (light.type == ELightType::analyticSphere)
    {
        return SphereSampler::sampleOrientationValid(surfacePoint, light);
    }
    else if (light.type == ELightType::analyticRectangle)
    {
        return RectangleSampler::sampleOrientationValid(surfacePoint, light, samplePoint);
    }
    else if (light.type == ELightType::constant)
    {
        return !allowIncompletePDF;
    }

    return false;
}

Optional<LightSample> sampleLights(const float3 p, const int lightId, const CoordinateSpace space, bool twoSided)
{
    LightSample lightSample;
    if ((lightId >= 0 && gPushConstants.lightCount == 1) || gPushConstants.lightCount == 0)
    {
        return none;
    }

    int lightIndex;

    do
    {
        lightIndex = min(int(Random::rand() * gPushConstants.lightCount), gPushConstants.lightCount - 1);
    } while (lightIndex == lightId);

    const float pdf = 1.0 / gPushConstants.lightCount;

    float distanceOverride = 1.0;

    const Light sampledLight = gLights[lightIndex];
    GeometrySample sample;
    if (sampledLight.type == ELightType::analyticSphere)
    {
        sample = SphereSampler::sampleSolidAngle(p, sampledLight);
    }
    else if (sampledLight.type == ELightType::analyticRectangle)
    {
        sample = RectangleSampler::sampleSolidAngle(p, sampledLight);
    }
    else if (sampledLight.type == ELightType::constant)
    {
        if (allowIncompletePDF)
        {
            return none;
        }
        sample = SphereSampler::sampleUniform(sampledLight.wCenter, 1.0, FOUR_PI);
        twoSided = true;
        distanceOverride = 2.0 * sampledLight.sceneRadius;
    }
    else
    {
        return none;
    }

    if (!lightSampleOrientationValid(lightIndex, p, sample.point))
    {
        return none;
    }

    lightSample.pdf = pdf * sample.pdf;

    float3 wDir               = (distanceOverride * sample.point) - p;
    const float pointDistance = length(wDir);
    wDir                      = normalize(wDir);

    lightSample.wi = space.worldToTangentFrame(wDir);
    float cosTheta = cosThetaTS(lightSample.wi);

    if (twoSided && cosTheta < 0.0)
    {
        cosTheta *= -1.0;
    }

    if (lightSample.pdf <= 0.0 || cosTheta <= 0.0)
    {
        return none;
    }

    ShadowRayPayload pld;
    pld.hit = true;
    RayDesc rayDescriptor;
    rayDescriptor.Origin    = p + space.normal * EPSILON;
    rayDescriptor.Direction = wDir;
    rayDescriptor.TMin      = 0.001;
    rayDescriptor.TMax      = (pointDistance - EPSILON) * 0.99;
    TraceRay(
        gTLAS,
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE, // FIXME: | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xff,                                                                                                // cullMask
        0,                                                                                                   // sbtRecordOffset
        0,                                                                                                   // sbtRecordStride
        1,                                                                                                   // missIndex
        rayDescriptor,
        pld);

    if (pld.hit)
    {
        return none;
    }

    lightSample.radiance = cosTheta * sampledLight.radiance * sampledLight.scale;

    if (lightSample.pdf <= 0.0)
    {
        return none;
    }

    return lightSample;
};

float3 getEnvironmentRadiance(const float3 wi)
{
    if (gPushConstants.environmentLightIdx == -1)
    {
        return float3(0);
    }

    const Light envLight = gLights[gPushConstants.environmentLightIdx];

    return envLight.radiance * envLight.scale;
}
