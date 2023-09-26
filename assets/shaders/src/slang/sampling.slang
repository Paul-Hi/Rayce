import globals;
import utils;
import data;
import random;

struct GeometrySample
{
    float3 point;
    float pdf;
};

extension Sphere
{
    GeometrySample sampleUniform()
    {
        GeometrySample sample;
        float2 uv = Random::rand2();
        float z = 1.0 - 2.0 * uv.x;
        float r = sqrt(max(0.0, 1.0 - z * z));
        float phi = TWO_PI * uv.y;
        float3 unitPoint = float3(r * cos(phi), r * sin(phi), z);
        sample.point = this.center + unitPoint * this.radius;
        sample.pdf = 1.0 / (2.0 * TWO_PI * this.radius * this.radius);
        return sample;
    }

    GeometrySample sampleSolidAngle(const float3 point)
    {
        GeometrySample sample;
        float3 wDir = this.center - point;
        const float dist = length(wDir);
        wDir = normalize(wDir);
        let space = CoordinateSpace(wDir);

        if(dist <= this.radius) // inside
        {
            return this.sampleUniform();
        }

        // max cone angles
        const float sinThetaMaxSqrd = this.radius * this.radius / (dist * dist);
        const float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMaxSqrd));
        sample.pdf = 1.0 / (TWO_PI * (1.0 - cosThetaMax));
        // random in cone
        const float2 uv = Random::rand2();
        const float cosTheta = (1.0 - uv.x) + uv.x * cosThetaMax;
        const float sinThetaSqrd = max(0.0, 1.0 - cosTheta * cosTheta);
        const float phi = TWO_PI * uv.y;
        // project to sphere surface
        const float cosAlpha = dist / this.radius * sinThetaSqrd + cosTheta * sqrt(max(0.0, 1.0 - dist * dist / (this.radius * this.radius) * sinThetaSqrd));
        const float sinAlpha = sqrt(max(0.0, 1.0 - cosAlpha * cosAlpha));
        float3 samplePoint = float3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
        samplePoint = space.flipped().tangentToWorld(samplePoint);

        sample.point = this.center + samplePoint * this.radius;

        return sample;
    }

    float samplePdf(const float3 point)
    {
        const float3 wDir = this.center - point;
        const float dist = length(wDir);

        const bool inside = (dist <= this.radius);

        if(inside)
        {
            return 1.0 / (2.0 * TWO_PI * this.radius * this.radius);
        }

        const float sinThetaMaxSqrd = this.radius * this.radius / (dist * dist);
        const float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMaxSqrd));

        return 1.0 / (TWO_PI * (1.0 - cosThetaMax));
    }
}

float pdfLight(const int lightId, const float3 surfacePoint)
{
    float pdf = 1.0 / gPushConstants.lightCount;
    const Light light = gLights.Load(lightId);

    if(light.type == ELightType::area && light.sphereId >= 0)
    {
        return gSpheres.Load(light.sphereId).samplePdf(surfacePoint);
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

    if(sampledLight.type == ELightType::area && sampledLight.sphereId >= 0)
    {
        Sphere sphere = gSpheres.Load(sampledLight.sphereId);

        GeometrySample sample = sphere.sampleSolidAngle(p);
        lightSample.pdf = sample.pdf;

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
    }

    lightSample.pdf = 0.0;
    lightSample.radiance = float3(0.0);
    lightSample.valid = false;
    return lightSample;
};
