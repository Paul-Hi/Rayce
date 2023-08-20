#ifndef SURFACE_STATE_GLSL
#define SURFACE_STATE_GLSL

struct BSDFParameters
{
        vec3 diffuseReflectance;
        bool twoSided;
        float interiorIor;
        float exteriorIor;
        vec3 specularReflectance;
        vec3 specularTransmittance;
        vec2 alpha;
        vec2 complexIor;
        bool nonlinear;
};

struct SurfaceState
{
    BSDFParameters bsdf;

    vec3 gNormal;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;

    uint canUseUv;

    vec3 wi; // in tangent space
    vec3 wo; // in tangent space
    vec3 wm; // in tangent space
};

SurfaceState surfaceState;

void populateSurfaceState(in Material material)
{
    surfaceState.gNormal = vec3(0.0);
    surfaceState.normal = vec3(0.0);
    surfaceState.tangent = vec3(0.0);
    surfaceState.bitangent = vec3(0.0);
    surfaceState.wi = vec3(0.0);
    surfaceState.wo = vec3(0.0);
    surfaceState.wm = vec3(0.0);

    surfaceState.bsdf.diffuseReflectance = material.diffuseReflectance;
    if (material.diffuseReflectanceTexture >= 0)
    {
        surfaceState.bsdf.diffuseReflectance = texture(textures[nonuniformEXT(material.diffuseReflectanceTexture)], pld.triangle.interpolatedUV).rgb;
    }

    surfaceState.gNormal = pld.triangle.geometryNormal;
    surfaceState.normal = pld.triangle.interpolatedNormal;
    createTBN(surfaceState.normal, (material.canUseUv == 1),
                pld.triangle.dfd1, pld.triangle.dfd2,
                pld.triangle.uvd1, pld.triangle.uvd2,
                surfaceState.tangent, surfaceState.bitangent);

    /*
    if(material.normalTextureId >= 0)
    {
        surfaceState.normal = normalize(texture(textures[nonuniformEXT(material.normalTextureId)], pld.triangle.interpolatedUV).rgb * 2.0 - 1.0);
        surfaceState.normal = normalize(tangentToWorld(surfaceState.normal, surfaceState.tangent, surfaceState.bitangent, surfaceState.gNormal));

        // recompute tangent frame - does this make sense?
        createTBN(surfaceState.normal, material.hasUV == 1,
                    pld.triangle.dfd1, pld.triangle.dfd2,
                    pld.triangle.uvd1, pld.triangle.uvd2,
                    surfaceState.tangent, surfaceState.bitangent);
    }
    */

    surfaceState.bsdf.alpha = material.alpha;
    if (material.alphaTexture >= 0)
    {
        surfaceState.bsdf.alpha = texture(textures[nonuniformEXT(material.alphaTexture)], pld.triangle.interpolatedUV).rg;
    }

    /*
    surfaceState.bsdf.emission = material.emissiveStrength * material.emissiveColor;
    if (material.emissiveTextureId >= 0)
    {
        surfaceState.bsdf.emission *= material.emissiveStrength * texture(textures[nonuniformEXT(material.emissiveTextureId)], pld.triangle.interpolatedUV).rgb;
    }
    */

    surfaceState.bsdf.twoSided = (material.twoSided == 1);
    surfaceState.bsdf.interiorIor = material.interiorIor;
    surfaceState.bsdf.exteriorIor = material.exteriorIor;
    surfaceState.bsdf.specularReflectance = material.specularReflectance;
    surfaceState.bsdf.specularTransmittance = material.specularTransmittance;
    surfaceState.bsdf.complexIor = material.complexIor;
    surfaceState.bsdf.nonlinear = (material.nonlinear == 1);
}

#endif // SURFACE_STATE_GLSL
