#ifndef SURFACE_STATE_GLSL
#define SURFACE_STATE_GLSL

// Helpful: https://schuttejoe.github.io/post/disneybsdf/

struct DisneyBSDFParameters
{
    // brdf
    vec3 baseColor;
    float metallic; // 0.0 - 1.0
    float subsurface; // 0.0 - 1.0
    float specular; // 0.0 - 1.0
    float roughness; // 0.0 - 1.0
    float specularTint; // 0.0 - 1.0
    float anisotropic; // 0.0 - 1.0
    float sheen; // 0.0 - 1.0
    float sheenTint; // 0.0 - 1.0
    float clearcoat; // 0.0 - 1.0
    float clearcoatGloss; // 0.0 - 1.0

    // bsdf
    float specTrans; // 0.0 - 1.0
    float ior;
    vec3 scatrDist;

    float flatness; // 0.0 - 1.0
    float diffTrans; // 0.0 - 1.0

    // not official?
    vec3 emission;
};

struct SurfaceState
{
    DisneyBSDFParameters bsdf;

    vec3 normal;
    vec3 tangent;
    vec3 bitangent;

};

SurfaceState surfaceState;

void populateSurfaceState(in Material material)
{
    // reset
    surfaceState.bsdf.baseColor = vec3(0.82, 0.67, 0.16);
    surfaceState.bsdf.metallic = 0.0;
    surfaceState.bsdf.subsurface = 0.0;
    surfaceState.bsdf.specular = 0.5;
    surfaceState.bsdf.roughness = 0.5;;
    surfaceState.bsdf.specularTint = 0.0;
    surfaceState.bsdf.anisotropic = 0.0;
    surfaceState.bsdf.sheen = 0.0;
    surfaceState.bsdf.sheenTint = 0.5;
    surfaceState.bsdf.clearcoat = 0.0;
    surfaceState.bsdf.clearcoatGloss = 1.0;
    surfaceState.bsdf.specTrans = 0.0;
    surfaceState.bsdf.ior = 1.5;
    surfaceState.bsdf.scatrDist = vec3(1.0);
    surfaceState.bsdf.flatness = 0.0;
    surfaceState.bsdf.diffTrans = 0.0;
    surfaceState.bsdf.emission = vec3(0.0);

    surfaceState.bsdf.baseColor = material.baseColor.rgb;
    if (material.baseColorTextureId >= 0)
    {
        surfaceState.bsdf.baseColor *= texture(textures[nonuniformEXT(material.baseColorTextureId)], pld.triangle.interpolatedUV).rgb;
    }

    surfaceState.normal = pld.triangle.interpolatedNormal;
    createTBN(pld.triangle.interpolatedNormal, material.hasUV == 1,
                pld.triangle.dfd1, pld.triangle.dfd2,
                pld.triangle.uvd1, pld.triangle.uvd2,
                surfaceState.tangent, surfaceState.bitangent);

    if(material.normalTextureId >= 0)
    {

        mat3 tbn = mat3(surfaceState.tangent, surfaceState.bitangent, surfaceState.normal);
        surfaceState.normal = normalize(texture(textures[nonuniformEXT(material.normalTextureId)], pld.triangle.interpolatedUV).rgb * 2.0 - 1.0);
        surfaceState.normal = normalize(tbn * surfaceState.normal.rgb);
    }

    surfaceState.bsdf.metallic = material.metallicFactor;
    surfaceState.bsdf.roughness = material.roughnessFactor;
    if (material.metallicRoughnessTextureId >= 0)
    {
        surfaceState.bsdf.metallic *= texture(textures[nonuniformEXT(material.metallicRoughnessTextureId)], pld.triangle.interpolatedUV).g;
        surfaceState.bsdf.roughness *= texture(textures[nonuniformEXT(material.metallicRoughnessTextureId)], pld.triangle.interpolatedUV).b;
    }

    surfaceState.bsdf.emission = material.emissiveStrength * material.emissiveColor;
    if (material.emissiveTextureId >= 0)
    {
        surfaceState.bsdf.emission *= material.emissiveStrength * texture(textures[nonuniformEXT(material.emissiveTextureId)], pld.triangle.interpolatedUV).rgb;
    }

    surfaceState.bsdf.specTrans = material.transmission;

    surfaceState.bsdf.ior = material.ior;
}


#endif // SURFACE_STATE_GLSL
