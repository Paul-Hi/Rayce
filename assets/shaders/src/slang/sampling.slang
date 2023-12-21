import globals;
import utils;
import data;
import random;

struct GeometrySample
{
    float3 point;
    float pdf;
};

bool finalizeSolidAngleSampling(inout float pdf, const float3 normal, const float3 point, const float3 samplePoint)
{
    float3 dir = point - samplePoint;
    float distSqrd = max(dot(dir, dir), EPSILON);
    float cosTheta = dot(normal, normalize(dir));
    if(cosTheta <= 0.0)
    {
        return false;
    }
    pdf *= distSqrd / (cosTheta);
    return true;
}

interface ISampler
{
    static GeometrySample sampleSolidAngle(const float3 point, const Light lightData);
    static float samplePdf(const float3 point, const Light lightData, const float3 samplePoint = float3(0.0));
};

struct SphereSampler : ISampler
{
    static GeometrySample sampleUniform(const float3 center, const float radius, const float surfaceArea)
    {
        GeometrySample sample;
        float2 uv = Random::rand2();
        float z = 1.0 - 2.0 * uv.x;
        float r = sqrt(max(0.0, 1.0 - z * z));
        float phi = TWO_PI * uv.y;
        float3 unitPoint = float3(r * cos(phi), r * sin(phi), z);
        sample.point = center + unitPoint * radius;
        sample.pdf = 1.0 / surfaceArea;
        return sample;
    }

    static GeometrySample sampleSolidAngle(const float3 point, const Light lightData)
    {
        GeometrySample sample;
        float3 wDir = lightData.wCenter - point;
        const float dist = length(wDir);
        wDir = normalize(wDir);
        let space = CoordinateSpace(wDir);

        const float radius = (mul(lightData.lightToWorld, float4(1.0, 0.0, 0.0, 1.0)).xyz - lightData.wCenter).x;

        if(dist <= radius) // inside
        {
            return SphereSampler::sampleUniform(lightData.wCenter, radius, lightData.surfaceArea);
        }

        // max cone angles
        const float sinThetaMaxSqrd = radius * radius / (dist * dist);
        const float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMaxSqrd));
        sample.pdf = 1.0 / (TWO_PI * (1.0 - cosThetaMax));
        // random in cone
        const float2 uv = Random::rand2();
        const float cosTheta = (1.0 - uv.x) + uv.x * cosThetaMax;
        const float sinThetaSqrd = max(0.0, 1.0 - cosTheta * cosTheta);
        const float phi = TWO_PI * uv.y;
        // project to sphere surface
        const float cosAlpha = dist / radius * sinThetaSqrd + cosTheta * sqrt(max(0.0, 1.0 - dist * dist / (radius * radius) * sinThetaSqrd));
        const float sinAlpha = sqrt(max(0.0, 1.0 - cosAlpha * cosAlpha));
        float3 samplePoint = float3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
        samplePoint = space.flipped().tangentToWorld(samplePoint);

        sample.point = lightData.wCenter + samplePoint * radius;

        return sample;
    }

    static float samplePdf(const float3 point, const Light lightData, const float3 samplePoint)
    {
        const float3 wDir = lightData.wCenter - point;
        const float dist = length(wDir);

        const float radius = (mul(lightData.lightToWorld, float4(1.0, 0.0, 0.0, 1.0)).xyz - lightData.wCenter).x;

        const bool inside = (dist <= radius);

        if(inside)
        {
            return 1.0 / lightData.surfaceArea;
        }

        const float sinThetaMaxSqrd = radius * radius / (dist * dist);
        const float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMaxSqrd));

        return 1.0 / (TWO_PI * (1.0 - cosThetaMax));
    }
}

struct RectangleSampler : ISampler
{
    static GeometrySample sampleSolidAngle(const float3 point, const Light lightData)
    {
        GeometrySample sample;

        // sample in -1, 1
        const float2 uv = Random::rand2() * 2.0 - 1.0;
        sample.point = mul(lightData.lightToWorld, float4(uv.x, 0.0, uv.y, 1.0)).xyz;


        sample.pdf = 1.0 / lightData.surfaceArea;

        float3 normal = normalize(mul(lightData.worldToLight, float4(0.0, 1.0, 0.0, 0.0)).xyz);
        if(finalizeSolidAngleSampling(sample.pdf, normal, point, sample.point))
        {
            return sample;
        }

        sample.pdf = 0.0;
        return sample;
    }

    static float samplePdf(const float3 point, const Light lightData, const float3 samplePoint)
    {
        float pdf = 1.0 / lightData.surfaceArea;

        float3 normal = normalize(mul(lightData.worldToLight, float4(0.0, 1.0, 0.0, 0.0)).xyz);
        if(finalizeSolidAngleSampling(pdf, normal, point, samplePoint))
        {
            return pdf;
        }

        return 0.0;
    }
}

float pdfLight(const int lightId, const float3 surfacePoint, const float3 samplePoint)
{
    float pdf = 1.0 / gPushConstants.lightCount;
    const Light light = gLights.Load(lightId);

    if(light.type == ELightType::analyticSphere)
    {
        return SphereSampler::samplePdf(surfacePoint, light) * pdf;
    }
    else if(light.type == ELightType::analyticRectangle)
    {
        return RectangleSampler::samplePdf(surfacePoint, light, samplePoint) * pdf;
    }

    return 0.0;
}

LightSample sampleLights(const float3 p, const int lightId, const CoordinateSpace space)
{
    LightSample lightSample;
    if(lightId >= 0 && gPushConstants.lightCount == 1)
    {
        lightSample.pdf = 0.0;
        lightSample.radiance = float3(0.0);
        lightSample.valid = false;
        return lightSample;
    }

    int lightIndex;

    do
    {
        lightIndex = int(Random::rand() * gPushConstants.lightCount);
    } while(lightIndex == lightId);

    Light sampledLight = gLights.Load(lightIndex);
    GeometrySample sample;
    if(sampledLight.type == ELightType::analyticSphere)
    {
        sample = SphereSampler::sampleSolidAngle(p, sampledLight);
        lightSample.pdf = sample.pdf;
    }
    else if(sampledLight.type == ELightType::analyticRectangle)
    {
        sample = RectangleSampler::sampleSolidAngle(p, sampledLight);
        lightSample.pdf = sample.pdf;
    }
    else
    {
        lightSample.pdf = 0.0;
        lightSample.radiance = float3(0.0);
        lightSample.valid = false;
        return lightSample;
    }

    float3 wDir = sample.point - p;
    float pointDistance = length(wDir);
    wDir = normalize(wDir);

    lightSample.wi = space.worldToTangentFrame(wDir);
    float nDotL = cosThetaTS(lightSample.wi);

    if(lightSample.pdf <= 0.0 || nDotL <= 0.0)
    {
        lightSample.pdf = 0.0;
        lightSample.radiance = float3(0.0);
        lightSample.valid = false;
        return lightSample;
    }

    RayPayload pld;
    pld.hit = true;
    RayDesc rayDescriptor;
    rayDescriptor.Origin = p + space.normal * EPSILON;
    rayDescriptor.Direction = wDir;
    rayDescriptor.TMin = 0.001;
    rayDescriptor.TMax = (pointDistance - EPSILON) * 0.99;
    TraceRay(
        gTLAS,
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE, // FIXME: | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xff,   // cullMask
        0,      // sbtRecordOffset
        0,      // sbtRecordStride
        0,      // missIndex
        rayDescriptor,
        pld
    );

    if(pld.hit)
    {
        lightSample.pdf = 0.0;
        lightSample.radiance = float3(0.0);
        lightSample.valid = false;
        return lightSample;
    }

    lightSample.radiance = nDotL * gPushConstants.lightCount * sampledLight.radiance;

    lightSample.valid = true;
    return lightSample;
};
