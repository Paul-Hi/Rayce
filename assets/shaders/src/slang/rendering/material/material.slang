import rendering.renderingUtils;
import rendering.material.ggx;
import rendering.material.instance.diffuse;
import rendering.material.instance.conductor;
import rendering.material.instance.dielectric;
import rendering.material.instance.plastic;
import core.data;

struct DiffuseMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.diffuseReflectance = materialData.diffuseReflectance;
        this.diffuseReflectanceTexture = materialData.diffuseReflectanceTexture;
    }

    typedef DiffuseMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 diffuseReflectance = this.diffuseReflectance;
        if (this.diffuseReflectanceTexture >= 0)
        {
            diffuseReflectance = gTextures[NonUniformResourceIndex(this.diffuseReflectanceTexture)].Sample(uv).rgb;
        }

        return DiffuseMaterialInstance(DiffuseBSDF(diffuseReflectance));
    }

    float3 diffuseReflectance;
    int diffuseReflectanceTexture;
};

struct SmoothConductorMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.etaTexture = materialData.conductorEtaTexture;
        this.kTexture = materialData.conductorKTexture;
        this.eta = materialData.conductorEta;
        this.k = materialData.conductorK;
    }

    typedef SmoothConductorMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 specularReflectance = this.specularReflectance;
        if (this.specularReflectanceTexture >= 0)
        {
            specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        float3 eta = this.eta;
        float3 k = this.k;
        if (this.etaTexture >= 0)
        {
            eta = gTextures[NonUniformResourceIndex(this.etaTexture)].Sample(uv).rgb;
        }
        if (this.kTexture >= 0)
        {
            k = gTextures[NonUniformResourceIndex(this.kTexture)].Sample(uv).rgb;
        }

        eta = max(eta, float3(EPSILON)); // FIXME: This could be handled better
        k = max(k, float3(EPSILON)); // FIXME: This could be handled better

        return SmoothConductorMaterialInstance(SmoothConductorBSDF(specularReflectance, eta, k));
    }

    float3 specularReflectance;
    int specularReflectanceTexture;
    float3 eta;
    int etaTexture;
    float3 k;
    int kTexture;
};

struct RoughConductorMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.etaTexture = materialData.conductorEtaTexture;
        this.kTexture = materialData.conductorKTexture;
        this.eta = materialData.conductorEta;
        this.k = materialData.conductorK;
        this.alpha = materialData.alpha.xy;
        this.alphaTexture = materialData.alphaTexture;
    }

    typedef RoughConductorMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 specularReflectance = this.specularReflectance;
        if (this.specularReflectanceTexture >= 0)
        {
            specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        float3 eta = this.eta;
        float3 k = this.k;
        if (this.etaTexture >= 0)
        {
            eta = gTextures[NonUniformResourceIndex(this.etaTexture)].Sample(uv).rgb;
        }
        if (this.kTexture >= 0)
        {
            k = gTextures[NonUniformResourceIndex(this.kTexture)].Sample(uv).rgb;
        }

        eta = max(eta, float3(EPSILON)); // FIXME: This could be handled better
        k = max(k, float3(EPSILON)); // FIXME: This could be handled better

        float alphaU = alpha.x;
        float alphaV = alpha.y;
        if (this.alphaTexture >= 0)
        {
            float2 alphaT = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            alphaU = alphaT.x;
            alphaV = alphaT.y;
        }

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        return RoughConductorMaterialInstance(RoughConductorBSDF(specularReflectance, eta, k, distr));
    }

    float3 specularReflectance;
    int specularReflectanceTexture;
    float3 eta;
    int etaTexture;
    float3 k;
    int kTexture;

    float2 alpha;
    int alphaTexture;
};

struct SmoothDielectricMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularTransmittance = materialData.specularTransmittance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.specularTransmittanceTexture = materialData.specularTransmittanceTexture;
        this.eta = materialData.interiorIor / materialData.exteriorIor;
    }

    typedef SmoothDielectricMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 specularReflectance = this.specularReflectance;
        float3 specularTransmittance = this.specularTransmittance;
        if (this.specularReflectanceTexture >= 0)
        {
            specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            specularTransmittance = gTextures[NonUniformResourceIndex(this.specularTransmittanceTexture)].Sample(uv).rgb;
        }

        float eta = this.eta;

        return SmoothDielectricMaterialInstance(SmoothDielectricBSDF(specularReflectance, specularTransmittance, eta));
    }

    float3 specularReflectance;
    int specularReflectanceTexture;
    float3 specularTransmittance;
    int specularTransmittanceTexture;
    float eta;
};

struct SmoothThinPlateDielectricMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularTransmittance = materialData.specularTransmittance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.specularTransmittanceTexture = materialData.specularTransmittanceTexture;
        this.eta = materialData.interiorIor / materialData.exteriorIor;
    }

    typedef SmoothThinPlateDielectricMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 specularReflectance = this.specularReflectance;
        float3 specularTransmittance = this.specularTransmittance;
        if (this.specularReflectanceTexture >= 0)
        {
            specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            specularTransmittance = gTextures[NonUniformResourceIndex(this.specularTransmittanceTexture)].Sample(uv).rgb;
        }

        float eta = this.eta;

        return SmoothThinPlateDielectricMaterialInstance(SmoothThinPlateDielectricBSDF(specularReflectance, specularTransmittance, eta));
    }

    float3 specularReflectance;
    int specularReflectanceTexture;
    float3 specularTransmittance;
    int specularTransmittanceTexture;
    float eta;
};

struct RoughDielectricMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularTransmittance = materialData.specularTransmittance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.specularTransmittanceTexture = materialData.specularTransmittanceTexture;
        this.eta = materialData.interiorIor / materialData.exteriorIor;
        this.alpha = materialData.alpha.xy;
        this.alphaTexture = materialData.alphaTexture;
    }

    typedef RoughDielectricMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 specularReflectance = this.specularReflectance;
        float3 specularTransmittance = this.specularTransmittance;
        if (this.specularReflectanceTexture >= 0)
        {
            specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            specularTransmittance = gTextures[NonUniformResourceIndex(this.specularTransmittanceTexture)].Sample(uv).rgb;
        }

        float eta = this.eta;

        float alphaU = alpha.x;
        float alphaV = alpha.y;
        if (this.alphaTexture >= 0)
        {
            float2 alphaT = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            alphaU = alphaT.x;
            alphaV = alphaT.y;
        }

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        return RoughDielectricMaterialInstance(RoughDielectricBSDF(specularReflectance, specularTransmittance, eta, distr));
    }

    float3 specularReflectance;
    int specularReflectanceTexture;
    float3 specularTransmittance;
    int specularTransmittanceTexture;
    float2 alpha;
    int alphaTexture;
    float eta;
};

struct SmoothPlasticMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.diffuseReflectance = materialData.diffuseReflectance;
        this.diffuseReflectanceTexture = materialData.diffuseReflectanceTexture;
        this.specularReflectance = materialData.specularReflectance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.eta = materialData.interiorIor / materialData.exteriorIor;
    }

    typedef SmoothPlasticMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 diffuseReflectance = this.diffuseReflectance;
        float3 specularReflectance = this.specularReflectance;
        if (this.diffuseReflectanceTexture >= 0)
        {
            diffuseReflectance = gTextures[NonUniformResourceIndex(this.diffuseReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularReflectanceTexture >= 0)
        {
            specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        const float eta = this.eta;

        return SmoothPlasticMaterialInstance(diffuseReflectance, specularReflectance, eta);
    }

    float eta;
    float3 diffuseReflectance;
    int diffuseReflectanceTexture;
    float3 specularReflectance;
    int specularReflectanceTexture;
};

struct RoughPlasticMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.diffuseReflectance = materialData.diffuseReflectance;
        this.diffuseReflectanceTexture = materialData.diffuseReflectanceTexture;
        this.specularReflectance = materialData.specularReflectance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.eta = materialData.interiorIor / materialData.exteriorIor;

        this.alpha = max(materialData.alpha.x, materialData.alpha.y);
        this.alphaTexture = materialData.alphaTexture;
    }

    typedef RoughPlasticMaterialInstance MaterialInstance;

    MaterialInstance getMaterialInstance(const float2 uv)
    {
        float3 diffuseReflectance = this.diffuseReflectance;
        float3 specularReflectance = this.specularReflectance;
        if (this.diffuseReflectanceTexture >= 0)
        {
            diffuseReflectance = gTextures[NonUniformResourceIndex(this.diffuseReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularReflectanceTexture >= 0)
        {
            specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        const float eta = this.eta;

        float alpha = this.alpha;
        if (this.alphaTexture >= 0)
        {
            float2 alphaT = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            alpha = max(alphaT.x, alphaT.y);
        }

        return RoughPlasticMaterialInstance(diffuseReflectance, specularReflectance, eta, alpha);
    }

    float eta;
    float3 diffuseReflectance;
    int diffuseReflectanceTexture;
    float3 specularReflectance;
    int specularReflectanceTexture;

    float alpha; // no anisotropy
    int alphaTexture;
};

struct DefaultAdapter : IAdapter
{
    __init(){};

    Optional<BxDFSample> sample(const IMaterialInstance bsdf, const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        return bsdf.sample(wo, mode, sampleFlags);
    }

    float3 f(const IMaterialInstance bsdf, const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return bsdf.f(wo, wi, mode);
    }

    float pdf(const IMaterialInstance bsdf, const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        return bsdf.pdf(wo, wi, mode, sampleFlags);
    }
};

struct TwoSidedAdapter : IAdapter
{
    __init(){};

    Optional<BxDFSample> sample(const IMaterialInstance bsdf, const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        bool flipped = false;
        float3 twoSidedWo = wo;
        if (cosThetaTS(twoSidedWo) < 0.0)
        {
            twoSidedWo.z *= -1;
            flipped = true;
        }

        const Optional<BxDFSample> result = bsdf.sample(twoSidedWo, mode, sampleFlags);

        if(result == none)
        {
            return none;
        }

        var bxdfSample = result.value;

        if (flipped)
        {
            bxdfSample.wi.z *= -1;
        }

        return bxdfSample;
    }

    float3 f(const IMaterialInstance bsdf, const float3 wo, const float3 wi, const ETransportMode mode)
    {
        float3 twoSidedWo = wo;
        float3 twoSidedWi = wi;
        if (cosThetaTS(wo) < 0.0)
        {
            twoSidedWo.z *= -1;
            twoSidedWi.z *= -1;
        }
        return bsdf.f(twoSidedWo, twoSidedWi, mode);
    }

    float pdf(const IMaterialInstance bsdf, const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        float3 twoSidedWo = wo;
        float3 twoSidedWi = wi;
        if (cosThetaTS(wo) < 0.0)
        {
            twoSidedWo.z *= -1;
            twoSidedWi.z *= -1;
        }
        return bsdf.pdf(twoSidedWo, twoSidedWi, mode, sampleFlags);
    }
};
