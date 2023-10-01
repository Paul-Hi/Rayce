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

    float Rs = negativeMultiplyAndAdd(data.etaIT, data.cosThetaT, absCosThetaI) /  multiplyAndAdd(data.etaIT, data.cosThetaT, absCosThetaI);
    float Rp =  negativeMultiplyAndAdd(data.etaIT, absCosThetaI, data.cosThetaT) /  multiplyAndAdd(data.etaIT, absCosThetaI, data.cosThetaT);

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

            sample.reflectance = specularReflectance / cosThetaTS(abs(sample.wi));

            return sample;
        }
        else
        {
            sample.wi = refractTS(wo, fresnelData.cosThetaT, fresnelData.etaTI);
            sample.pdf = 1.0 - fresnelData.F;

            sample.reflectance = fresnelData.etaTI * fresnelData.etaTI * specularTransmittance / cosThetaTS(abs(sample.wi));

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

            sample.reflectance = specularReflectance / cosThetaTS(abs(sample.wi));

            return sample;
        }
        else
        {
            sample.wi = refractTS(wo, fresnelData.cosThetaT, fresnelData.etaTI);
            sample.pdf = 1.0 - fresnelData.F;

            sample.reflectance = specularTransmittance / cosThetaTS(abs(sample.wi));

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
            brdf.diffuseReflectance = gTextures[this.diffuseReflectanceTexture].Sample(gSamplers[this.diffuseReflectanceTexture], uv).rgb;
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
            bsdf.specularReflectance = gTextures[this.specularReflectanceTexture].Sample(gSamplers[this.specularReflectanceTexture], uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            bsdf.specularTransmittance = gTextures[this.specularTransmittanceTexture].Sample(gSamplers[this.specularTransmittanceTexture], uv).rgb;
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
            bsdf.specularReflectance = gTextures[this.specularReflectanceTexture].Sample(gSamplers[this.specularReflectanceTexture], uv).rgb;
        }
        if (this.specularTransmittanceTexture >= 0)
        {
            bsdf.specularTransmittance = gTextures[this.specularTransmittanceTexture].Sample(gSamplers[this.specularTransmittanceTexture], uv).rgb;
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

extension Material
{
    IMaterial getMaterial()
    {
        switch(this.bxdfType)
        {
            case EBxDFType::diffuse:
                {
                    // FIXME: The Problem is creating something dynamic is not possible (conformances - where and how to add?)
                    return createDynamicObject<IMaterial>(uint(EBxDFType::diffuse), DiffuseMaterial(this));
                }
            case EBxDFType::smoothDielectric:
                {
                    return createDynamicObject<IMaterial>(uint(EBxDFType::smoothDielectric), SmoothDielectricMaterial(this));
                }
            case EBxDFType::smoothDielectricThin:
                {
                    return createDynamicObject<IMaterial>(uint(EBxDFType::smoothDielectricThin), SmoothThinDielectricMaterial(this));
                }
            default:
                {
                    return createDynamicObject<IMaterial>(uint(EBxDFType::diffuse), DiffuseMaterial(this));
                }
        }
    }
}