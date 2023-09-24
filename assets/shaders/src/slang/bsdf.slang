import globals;
import utils;
import random;

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
