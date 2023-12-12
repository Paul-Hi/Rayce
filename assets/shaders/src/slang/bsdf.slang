import globals;
import utils;
import random;
import data;

interface IBxDF
{
    EBxDFType getType();

    BxDFSample sample(const float3 wo);
    float3 evaluate(const float3 wo, const float3 wi);
    float pdf(const float3 wo, const float3 wi);

    bool isDirac();

};

struct DielectricFresnelData
{
    float F;
    float cosThetaI;
    float cosThetaT;
    float etaIT;
    float etaTI;
};

float3 cosineHemisphereSample()
{
    float2 uv = Random::rand2();
    float phi = TWO_PI * uv.x;
    float cosTheta = sqrt(1.0 - uv.y);
    float sinTheta = sqrt(uv.y);
    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float z = cosTheta;

    return float3(x, y, z);
}

float cosineHemispherePdf(const float3 wi)
{
    return cosThetaTS(wi) * INV_PI;
}

DielectricFresnelData FresnelDielectric(const float cosThetaI, const float eta)
{
    DielectricFresnelData data;
    data.cosThetaI = cosThetaI;
    const bool switchI = (cosThetaI < 0.0);
    data.etaIT = switchI ? 1.0 / eta : eta;
    data.etaTI = switchI ? eta : 1.0 / eta;

    if(eta == 1.0)
    {
        data.cosThetaT = -cosThetaI;
        data.F = 0.0;
        return data;
    }

    float cosThetaTSqrd = negativeMultiplyAndAdd(negativeMultiplyAndAdd(cosThetaI, cosThetaI, 1.0), data.etaTI * data.etaTI, 1.0);

    // total internal reflection
    if (cosThetaTSqrd <= 0.0)
    {
        data.cosThetaT = 0.0;
        data.F = 1.0;
        return data;
    }

    float absCosThetaI = abs(cosThetaI);
    data.cosThetaT = sqrt(cosThetaTSqrd);

    float Rs = negativeMultiplyAndAdd(data.etaIT, data.cosThetaT, absCosThetaI) / multiplyAndAdd(data.etaIT, data.cosThetaT, absCosThetaI);
    float Rp = negativeMultiplyAndAdd(data.etaIT, absCosThetaI, data.cosThetaT) / multiplyAndAdd(data.etaIT, absCosThetaI, data.cosThetaT);

    data.cosThetaT = !switchI ? -data.cosThetaT : data.cosThetaT;

    data.F = 0.5 * (Rs * Rs + Rp * Rp);

    return data;
}

struct DiffuseBRDF : IBxDF
{
    BxDFSample sample(const float3 wo)
    {
        BxDFSample sample;
        sample.dirac = isDirac();

        sample.wi = cosineHemisphereSample();
        if(wo.z < 0)
        {
            sample.wi.z *= -1;
        }

        sample.pdf = cosineHemispherePdf(abs(sample.wi));

        sample.reflectance = evaluate(wo, sample.wi);

        return sample;
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        float nDotV = cosThetaTS(wo);
        if(nDotV <= 0.0)
        {
            return float3(0.0);
        }
        // lambertian
        return diffuseReflectance * INV_PI;
    }

    float pdf(const float3 wo, const float3 wi)
    {
        float nDotV = cosThetaTS(wo);
        if(nDotV <= 0.0)
        {
            return 0.0;
        }
        return cosThetaTS(wi) / INV_PI;
    }

    bool isDirac()
    {
        return false;
    };

    EBxDFType getType()
    {
        return EBxDFType::diffuse;
    }

    float3 diffuseReflectance;
};

struct SmoothDielectricBSDF : IBxDF
{
    BxDFSample sample(const float3 wo)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        const float cosThetaI = cosThetaTS(wo);
        const float eta = interiorIor / exteriorIor;
        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaI, eta);

        if (Random::rand() <= fresnelData.F)
        {
            sample.wi = reflectTS(wo);
            sample.pdf = fresnelData.F;

            const float absNDotL = abs(cosThetaTS(sample.wi));
            sample.reflectance = absNDotL > 0.0 ? specularReflectance / absNDotL : float3(0.0);

            return sample;
        }
        else
        {
            sample.wi = refractTS(wo, fresnelData.cosThetaT, fresnelData.etaTI);
            sample.pdf = 1.0 - fresnelData.F;

            const float absNDotL = abs(cosThetaTS(sample.wi));
            sample.reflectance = absNDotL > 0.0 ? fresnelData.etaTI * fresnelData.etaTI * specularTransmittance / absNDotL : float3(0.0);

            return sample;
        }
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        return float3(0.0);
    }

    float pdf(const float3 wo, const float3 wi)
    {
        return 0.0;
    }

    bool isDirac()
    {
        return true;
    };

    EBxDFType getType()
    {
        return EBxDFType::smoothDielectric;
    }

    float interiorIor, exteriorIor;
    float3 specularReflectance;
    float3 specularTransmittance;
};

struct SmoothThinDielectricBSDF : IBxDF
{
    BxDFSample sample(const float3 wo)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        const float cosThetaI = cosThetaTS(wo);
        const float eta = interiorIor / exteriorIor;
        DielectricFresnelData fresnelData = FresnelDielectric(abs(cosThetaI), eta);

        // Account for internal reflections
        fresnelData.F *= 2.0 / (1.0 + fresnelData.F);

        if (Random::rand() <= fresnelData.F)
        {
            sample.wi = reflectTS(wo);
            sample.pdf = fresnelData.F;

            const float absNDotL = abs(cosThetaTS(sample.wi));
            sample.reflectance = absNDotL > 0.0 ? specularReflectance / absNDotL : float3(0.0);

            return sample;
        }
        else
        {
            sample.wi = refractTS(wo, fresnelData.cosThetaT, fresnelData.etaTI);
            sample.pdf = 1.0 - fresnelData.F;

            const float absNDotL = abs(cosThetaTS(sample.wi));
            sample.reflectance = absNDotL > 0.0 ? specularTransmittance / absNDotL : float3(0.0);

            return sample;
        }
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        return float3(0.0);
    }

    float pdf(const float3 wo, const float3 wi)
    {
        return 0.0;
    }

    bool isDirac()
    {
        return true;
    };

    EBxDFType getType()
    {
        return EBxDFType::smoothDielectricThin;
    }

    float interiorIor, exteriorIor;
    float3 specularReflectance;
    float3 specularTransmittance;
};

struct GGXDistribution
{
    __init(float alphaU, float alphaV)
    {
        this.alphaU = alphaU;
        this.alphaV = alphaV;
    }

    // VNDF
    float evaluate(const float3 wm)
    {
        float tan2Theta = tan2ThetaTS(wm);
        if (isinf(tan2Theta))
        {
            return 0.0;
        }
        float cos4Theta = cos2ThetaTS(wm);
        cos4Theta *= cos4Theta;
        if (cos4Theta < EPSILON)
        {
            return 0.0;
        }

        float scldAlphaU = cosPhiTS(wm) / alphaU;
        float scldAlphaV = sinPhiTS(wm) / alphaV;
        float sqrdAlpha = scldAlphaU * scldAlphaU + scldAlphaV * scldAlphaV;
        float e = tan2Theta * sqrdAlpha;

        return 1.0 / (PI * alphaU * alphaV * cos4Theta * (1.0 + e) * (1.0 + e));
    }

    float evaluate(const float3 wo, const float3 wm)
    {
        return G1(wo) / abs(cosThetaTS(wo)) * abs(dot(wo, wm)) * evaluate(wm);
    }

    float lambda(const float3 w)
    {
        float tan2Theta = tan2ThetaTS(w);
        if (isinf(tan2Theta))
        {
            return 0.0;
        }

        float scldAlphaU = cosPhiTS(w) * alphaU;
        float scldAlphaV = sinPhiTS(w) * alphaV;
        float sqrdAlpha = scldAlphaU * scldAlphaU + scldAlphaV * scldAlphaV;
        return (sqrt(1.0 + sqrdAlpha * tan2Theta) - 1.0) * 0.5;
    }

    // based on Sampling the GGX Distribution of Visible Normals https://jcgt.org/published/0007/04/01/
    MicrofacetSample sample(const float3 wo)
    {
        // sample visible
        MicrofacetSample sample;

        float2 rand = Random::rand2();

        float3 stretchedWo = normalize(float3(alphaU * wo.x, alphaV * wo.y, wo.z));

        float sqrdLen = stretchedWo.x * stretchedWo.x + stretchedWo.y * stretchedWo.y;
        float3 T1 = sqrdLen > 0 ? float3(-stretchedWo.y, stretchedWo.x, 0.0) * (1.0 / sqrt(sqrdLen)) : float3(1.0, 0.0, 0.0);
        float3 T2 = cross(stretchedWo, T1);

        float r = sqrt(rand.x);
        float phi = TWO_PI * rand.y;
        float t1 = r * cos(phi);
        float t2 = r * sin(phi);
        float s = 0.5 * (1.0 + stretchedWo.z);
        t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

        float3 proj = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * stretchedWo;

        sample.wm = normalize(float3(alphaU * proj.x, alphaV * proj.y, max(0.0, proj.z)));

        sample.pdf = pdf(stretchedWo, sample.wm);

        return sample;
    }

    float G1(const float3 w)
    {
        return 1.0 / (1.0 + lambda(w));
    }

    float G(const float3 wi, const float3 wo)
    {
        return 1.0 / (1.0 + lambda(wi) + lambda(wo));
    }

    float pdf(const float3 wo, const float3 wm)
    {
        return evaluate(wo, wm);
    }

    float alphaU, alphaV;
};

struct RoughDielectricBSDF : IBxDF
{
    BxDFSample sample(const float3 wo)
    {
        BxDFSample sample;
        sample.dirac = isDirac();

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float cosThetaI = cosThetaTS(wo);
        const MicrofacetSample mfSample = distr.sample(sign(cosThetaI) * wo);
        if (mfSample.pdf == 0.0)
        {
            sample.wi = float3(0.0);
            sample.reflectance = float3(0.0);
            sample.pdf = 0.0;
            return sample;
        }

        const float D = distr.evaluate(mfSample.wm);

        if(D == 0.0)
        {
            sample.wi = float3(0.0);
            sample.reflectance = float3(0.0);
            sample.pdf = 0.0;
            return sample;
        }

        const float eta = interiorIor / exteriorIor;
        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wo, mfSample.wm), eta);

        if (Random::rand() <= fresnelData.F)
        {
            sample.wi = reflectTS(wo);

            sample.pdf = mfSample.pdf * fresnelData.F / (4.0 * abs(dot(wo, mfSample.wm)));

            const float G = distr.G(sample.wi, wo);
            sample.reflectance = specularReflectance * D * G * fresnelData.F / abs(4.0 * cosThetaTS(sample.wi) * cosThetaI);

            return sample;
        }
        else
        {
            sample.wi = refractTS(wo, fresnelData.cosThetaT, fresnelData.etaTI);

            float sqrtDenom = dot(sample.wi, mfSample.wm) + dot(wo, mfSample.wm) / fresnelData.etaTI;
            float dwmDwi = abs(dot(sample.wi, mfSample.wm)) / (sqrtDenom * sqrtDenom);

            sample.pdf = mfSample.pdf * (1.0 - fresnelData.F) * dwmDwi;

            dwmDwi = abs(dot(sample.wi, mfSample.wm) * dot(wo, mfSample.wm) / (sqrtDenom * sqrtDenom * cosThetaTS(sample.wi) * cosThetaI));

            const float G = distr.G(sample.wi, wo);
            sample.reflectance = specularTransmittance * D * G * (1.0 - fresnelData.F) * dwmDwi / (fresnelData.etaTI * fresnelData.etaTI);

            return sample;
        }
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        bool isReflection = cosThetaI * cosThetaO > 0.0;

        float eta = interiorIor / exteriorIor;
        float etaP = isReflection ? 1.0 : (cosThetaO > 0.0 ? eta : 1.0 / eta);

        float3 wm = etaP * wi + wo;

        if(length(wm) * length(wm) < EPSILON)
        {
            return float3(0.0);
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        if (dot(wm, wi) * cosThetaI < 0.0 || dot(wm, wo) * cosThetaO < 0.0)
        {
            return float3(0.0);
        }

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float D = distr.evaluate(wm);

        if(D == 0.0)
        {
            return float3(0.0);
        }

        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wo, wm), eta);

        const float G = distr.G(wi, wo);

        if (isReflection)
        {
            return specularReflectance * D * G * fresnelData.F / abs(4.0 * cosThetaO * cosThetaI);
        }
        else
        {
            float sqrtDenom = (dot(wi, wm) + dot(wo, wm) / etaP);
            float dwmDwi = abs(dot(wi, wm) * dot(wo, wm) / (sqrtDenom * sqrtDenom * cosThetaO * cosThetaI));

            return specularTransmittance * D * G * (1.0 - fresnelData.F) * dwmDwi / (etaP * etaP);
        }
    }

    float pdf(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        bool isReflection = cosThetaI * cosThetaO > 0.0;

        float eta = interiorIor / exteriorIor;
        float etaP = isReflection ? 1.0 : (cosThetaO > 0.0 ? eta : 1.0 / eta);

        float3 wm = etaP * wi + wo;

        if(cosThetaO == 0.0 || cosThetaI == 0.0 || length(wm) * length(wm) < EPSILON)
        {
            return 0.0;
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        if (dot(wm, wi) * cosThetaI < 0.0 || dot(wm, wo) * cosThetaO < 0.0)
        {
            return 0.0;
        }

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wo, wm), eta);

        if (isReflection)
        {
            return distr.pdf(sign(cosThetaO) * wo, wm) * fresnelData.F / abs(4.0 * abs(dot(wo, wm)));
        }
        else
        {
            float sqrtDenom = dot(wi, wm) + dot(wo, wm) / etaP;
            float dwmDwi = abs(dot(wi, wm)) / (sqrtDenom * sqrtDenom);

            return distr.pdf(sign(cosThetaO) * wo, wm) * (1.0 - fresnelData.F) * dwmDwi;
        }
    }

    bool isDirac()
    {
        return false;
    };

    EBxDFType getType()
    {
        return EBxDFType::roughDielectric;
    }

    float interiorIor, exteriorIor;
    float3 specularReflectance;
    float3 specularTransmittance;

    float alphaU, alphaV;
    // bool sampleVisible = true; // FIXME: Add option?
    // Distribution distribution = Distribution::GGX; // FIXME: Add option?
};

interface IMaterial
{
    associatedtype RealBxDF : IBxDF;
    RealBxDF createBxDF(const float2 uv);
};

struct DiffuseMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.diffuseReflectance = materialData.diffuseReflectance;
        this.diffuseReflectanceTexture = materialData.diffuseReflectanceTexture;
    }

    typedef DiffuseBRDF RealBxDF;

    DiffuseBRDF createBxDF(const float2 uv)
    {
        DiffuseBRDF brdf;
        brdf.diffuseReflectance = this.diffuseReflectance;
        if (this.diffuseReflectanceTexture >= 0)
        {
            brdf.diffuseReflectance = gTextures[NonUniformResourceIndex(this.diffuseReflectanceTexture)].Sample(uv).rgb;
        }

        return brdf;
    }

    float3 diffuseReflectance;
    int diffuseReflectanceTexture;
};

struct SmoothDielectricMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularTransmittance = materialData.specularTransmittance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.specularTransmittanceTexture = materialData.specularTransmittanceTexture;
        this.interiorIor = materialData.interiorIor;
        this.exteriorIor = materialData.exteriorIor;
    }

    typedef SmoothDielectricBSDF RealBxDF;

    SmoothDielectricBSDF createBxDF(const float2 uv)
    {
        SmoothDielectricBSDF bsdf;
        bsdf.specularReflectance = this.specularReflectance;
        bsdf.specularTransmittance = this.specularTransmittance;
        if (this.specularReflectanceTexture >= 0)
        {
            bsdf.specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            bsdf.specularTransmittance = gTextures[NonUniformResourceIndex(this.specularTransmittanceTexture)].Sample(uv).rgb;
        }

        bsdf.interiorIor = this.interiorIor;
        bsdf.exteriorIor = this.exteriorIor;

        return bsdf;
    }

    float3 specularReflectance;
    float3 specularTransmittance;
    int specularReflectanceTexture;
    int specularTransmittanceTexture;
    float interiorIor;
    float exteriorIor;
};

struct SmoothThinDielectricMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularTransmittance = materialData.specularTransmittance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.specularTransmittanceTexture = materialData.specularTransmittanceTexture;
        this.interiorIor = materialData.interiorIor;
        this.exteriorIor = materialData.exteriorIor;
    }

    typedef SmoothThinDielectricBSDF RealBxDF;

    SmoothThinDielectricBSDF createBxDF(const float2 uv)
    {
        SmoothThinDielectricBSDF bsdf;
        bsdf.specularReflectance = this.specularReflectance;
        bsdf.specularTransmittance = this.specularTransmittance;
        if (this.specularReflectanceTexture >= 0)
        {
            bsdf.specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            bsdf.specularTransmittance = gTextures[NonUniformResourceIndex(this.specularTransmittanceTexture)].Sample(uv).rgb;
        }

        bsdf.interiorIor = this.interiorIor;
        bsdf.exteriorIor = this.exteriorIor;

        return bsdf;
    }

    float3 specularReflectance;
    float3 specularTransmittance;
    int specularReflectanceTexture;
    int specularTransmittanceTexture;
    float interiorIor;
    float exteriorIor;
};

struct RoughDielectricMaterial : IMaterial
{
    __init(const Material materialData)
    {
        this.specularReflectance = materialData.specularReflectance;
        this.specularTransmittance = materialData.specularTransmittance;
        this.specularReflectanceTexture = materialData.specularReflectanceTexture;
        this.specularTransmittanceTexture = materialData.specularTransmittanceTexture;
        this.interiorIor = materialData.interiorIor;
        this.exteriorIor = materialData.exteriorIor;
        this.alpha = materialData.alpha;
        this.alphaTexture = materialData.alphaTexture;
    }

    typedef RoughDielectricBSDF RealBxDF;

    RoughDielectricBSDF createBxDF(const float2 uv)
    {
        RoughDielectricBSDF bsdf;
        bsdf.specularReflectance = this.specularReflectance;
        bsdf.specularTransmittance = this.specularTransmittance;
        if (this.specularReflectanceTexture >= 0)
        {
            bsdf.specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            bsdf.specularTransmittance = gTextures[NonUniformResourceIndex(this.specularTransmittanceTexture)].Sample(uv).rgb;
        }

         bsdf.alphaU = alpha.x;
         bsdf.alphaV = alpha.y;
        if (this.alphaTexture >= 0)
        {
            float2 alphaT = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            bsdf.alphaU = alphaT.x;
            bsdf.alphaV = alphaT.y;
        }

        bsdf.interiorIor = this.interiorIor;
        bsdf.exteriorIor = this.exteriorIor;

        bsdf.alphaU = max(bsdf.alphaU, EPSILON); // FIXME: This could be handled better
        bsdf.alphaV = max(bsdf.alphaV, EPSILON); // FIXME: This could be handled better

        return bsdf;
    }

    float3 specularReflectance;
    float3 specularTransmittance;
    int specularReflectanceTexture;
    int specularTransmittanceTexture;
    float interiorIor;
    float exteriorIor;

    float2 alpha;
    int alphaTexture;
};

extension Material
{
    IMaterial getMaterial()
    {
        switch(this.bxdfType)
        {
            case EBxDFType::diffuse:
                {
                    // FIXME: The Problem is creating something dynamic is not possible (conformances - where and how to add?)
                    return DiffuseMaterial(this);
                }
            case EBxDFType::smoothDielectric:
                {
                    return SmoothDielectricMaterial(this);
                }
            case EBxDFType::smoothDielectricThin:
                {
                    return SmoothThinDielectricMaterial(this);
                }
            case EBxDFType::roughDielectric:
                {
                    bool rough = (length(this.alpha) > 0.0) || this.alphaTexture >= 0;
                    if(!rough)
                    {
                        return SmoothDielectricMaterial(this);
                    }

                    return RoughDielectricMaterial(this);
                }
            default:
                {
                    return DiffuseMaterial(this);
                }
        }
    }
}