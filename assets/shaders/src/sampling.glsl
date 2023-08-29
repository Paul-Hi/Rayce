#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#include "random.glsl"

vec3 sampleSphereUniform(in Sphere sphere)
{
    float z = 1.0 - 2.0 * rand();
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * rand();
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 sampleSphereSolidAngle(in vec3 point, in Sphere sphere, out float pdf)
{
        vec3 wDir = sphere.center - point;
        float dist = length(wDir);
        wDir = normalize(wDir);
        vec3 wDx, wDy;
        createCoordinateSystem(wDir, wDx, wDy);

        bool inside = (dist <= sphere.radius);
        if(inside)
        {
            pdf = 1.0 / (2.0 * TWO_PI * sphere.radius * sphere.radius);
            vec3 samplePoint = sampleSphereUniform(sphere);
            return sphere.center + samplePoint * sphere.radius;
        }

        // max cone angles
        float sinThetaMaxSqrd = sphere.radius * sphere.radius / (dist * dist);
        float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMaxSqrd));
        pdf = 1.0 / (TWO_PI * (1.0 - cosThetaMax));
        // random in cone
        float rand0 = rand();
        float cosTheta = (1.0 - rand0) + rand0 * cosThetaMax;
        float sinThetaSqrd = max(0.0, 1.0 - cosTheta * cosTheta);
        float phi = TWO_PI * rand();
        // project to sphere surface
        float cosAlpha = dist / sphere.radius * sinThetaSqrd + cosTheta * sqrt(max(0.0, 1.0 - dist * dist / (sphere.radius * sphere.radius) * sinThetaSqrd));
        float sinAlpha = sqrt(max(0.0, 1.0 - cosAlpha * cosAlpha));
        vec3 samplePoint = vec3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
        samplePoint = tangentToWorld(samplePoint, -wDx, -wDy, -wDir);

        return sphere.center + samplePoint * sphere.radius;
}

float pdfLight(in int lightId, in vec3 surfacePoint)
{
    float pdf = 1.0 / lightCount;
    Light light = lights[lightId];

    if(light.type == area && light.sphereId >= 0)
    {
        Sphere sphere = spheres[light.sphereId];
        vec3 wDir = sphere.center - surfacePoint;
        float dist = length(wDir);

        bool inside = (dist <= sphere.radius);
        if(inside)
        {
            return pdf / (2.0 * TWO_PI * sphere.radius * sphere.radius);
        }

        float sinThetaMaxSqrd = sphere.radius * sphere.radius / (dist * dist);
        float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMaxSqrd));
        return pdf / (TWO_PI * (1.0 - cosThetaMax));
    }

    return 0.0;
}

bool sampleLights(in vec3 p, in int pLightId, out LightSample lightSample)
{
    if(pLightId >= 0 && lightCount == 1)
    {
        return false;
    }

    int lightIndex;

    do
    {
        lightIndex = int(rand() * lightCount);
    } while(lightIndex == pLightId);

    Light sampledLight = lights[lightIndex];

    if(sampledLight.type == area && sampledLight.sphereId >= 0)
    {
        Sphere sphere = spheres[sampledLight.sphereId];

        vec3 samplePoint = sampleSphereSolidAngle(p, sphere, lightSample.pdf);

        vec3 wDir = samplePoint - p;
        float pointDistance = length(wDir);
        wDir = normalize(wDir);

        surfaceState.wi = worldToTangentFrame(wDir, surfaceState.tangent, surfaceState.bitangent, surfaceState.normal);
        float nDotL = cosTheta(surfaceState.wi);

        if(lightSample.pdf <= 0.0 || nDotL <= 0.0)
        {
            return false;
        }

        pld.hit = true;
        traceRayEXT(
            TLAS,
            gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsCullBackFacingTrianglesEXT,
            0xff,   // cullMask
            0,      // sbtRecordOffset
            0,      // sbtRecordStride
            0,      // missIndex
            p,
            0.001,
            wDir,
            pointDistance * 0.99,
            0       // payloadLocation
        );

        if(pld.hit)
        {
            return false;
        }

        lightSample.radiance = nDotL * lightCount * sampledLight.radiance;

        return true;
    }

    return false;
};

#endif // SAMPLING_GLSL
