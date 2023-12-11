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

    float evaluate(const float3 wm)
    {
        const float cos2Theta = cos2ThetaTS(wm);
        const float cos2Phi = cos2PhiTS(wm);
        const float sin2Phi = sin2PhiTS(wm);

        float e = (cos2Phi / (alphaU * alphaU) + sin2Phi / (alphaV * alphaV)) / cos2Theta;

        // GGX
        float root = (1.0 + e) * cos2Theta;
        float ggx =  1.0 / (PI * alphaU * alphaV * root * root);


        // Should prevent potential numerical issues
        if (ggx * cosThetaTS(wm) < 1e-20)
        {
            ggx = 0.0;
        }

        return ggx;
    }

    // based on pbr book and mitsuba
    MicrofacetSample sample(const float3 wo)
    {
        // sample visible
        MicrofacetSample sample;

        // stretched wo
        float3 stretchedWo = normalize(float3(alphaU * wo.x, alphaV * wo.y, wo.z));

        float2 slope = sample11(cosThetaTS(stretchedWo));

        // rotate
        const float cosPhi = cosPhiTS(stretchedWo);
        const float sinPhi = sinPhiTS(stretchedWo);

        slope = float2(cosPhi * slope.x - sinPhi * slope.y, sinPhi * slope.x + cosPhi * slope.y);

        // unstretch
        slope.x *= alphaU;
        slope.y *= alphaV;

        sample.wm = normalize(float3(-slope.x, -slope.y, 1.0));

        sample.pdf = pdf(wo, sample.wm);

        return sample;
    }

    float smithG1(const float3 w, const float3 wm)
    {
        if(dot(w, wm) * cosThetaTS(w) <= 0.0)
        {
            return 0.0;
        }

        const float tanTheta = abs(tanThetaTS(w));
        if(tanTheta == 0.0)
        {
            return 1.0;
        }

        float a = projectRoughness(w);

        float root = a * tanTheta;

        return 2.0 / (1.0 + hypot2(1.0, root));

    }

    float projectRoughness(const float3 w)
    {
        float invSin2Theta = 1.0 / sin2ThetaTS(w);

        if (alphaU == alphaV || invSin2Theta <= 0)
            return alphaU;

        float cos2Phi = w.x * w.x * invSin2Theta; // To not compute invSin2Theta again
        float sin2Phi = w.y * w.y * invSin2Theta; // To not compute invSin2Theta again

        return sqrt(cos2Phi * alphaU * alphaU + sin2Phi * alphaV * alphaV);
    }

    float2 sample11(float cosTheta)
    {
        float2 result;
        float2 rand = Random::rand2();

        if(cosTheta > 0.9999)
        {
            // normal incidence
            float r = sqrt(rand.x / (1.0 - rand.x)); // FIXME: Safe?
            float phi = TWO_PI * rand.y;
            return float2(r * cos(phi), r * sin(phi));
        }

        float sinTheta = max(sqrt(1.0 - cosTheta * cosTheta), 0.0);

        float tanTheta = sinTheta / cosTheta;

        float a = 1.0 / tanTheta;

        float G1 = 2.0 / (1.0 + sqrt(1.0 + 1.0 / (a + a)));

        // sample slope x
        float A = 2.0 * rand.x / G1 - 1.0;
        if (abs(A) == 1)
        {
            A -= sign(A) * EPSILON;
        }

        float tmp = 1.0 / (A * A - 1.0);

        float B = tanTheta;
        float D = sqrt(max(B * B * tmp * tmp - (A * A - B * B ) * tmp, 0.0));

        float2 slopeX = float2(B * tmp - D, B * tmp + D);
        result.x = (A < 0|| slopeX.y > 1.0 / tanTheta) ? slopeX.x : slopeX.y;

        // sample slope y
        float S;
        if(rand.y > 0.5)
        {
            S = 1.0;
            rand.y = 2.0 * (rand.y - 0.5);
        }
        else
        {
            S = -1.0;
            rand.y = 2.0 * (0.5 - rand.y);
        }

        // improved fit from mitsuba
        float z =
            (rand.y * (rand.y * (rand.y * (-0.365728915865723) + 0.790235037209296)
                        - 0.424965825137544) + 0.000152998850436920) /
                        (rand.y * (rand.y * (rand.y * (rand.y * 0.169507819808272 - 0.397203533833404)
                        - 0.232500544458471) + 1.0) - 0.539825872510702);

        result.y = S * z * sqrt(1.0 + result.x * result.x);

        return result;
    }

    float pdf(const float3 wo, const float3 wm)
    {
        const float cosTheta = cosThetaTS(wo);
        if(cosTheta == 0.0)
        {
            return 0.0;
        }

        return smithG1(wo, wm) * abs(dot(wo, wm)) * evaluate(wm) / cosTheta;
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

        const float G = distr.smithG1(wo, mfSample.wm);

        if (Random::rand() <= fresnelData.F)
        {
            sample.wi = reflectTS(wo);
            sample.pdf = mfSample.pdf * fresnelData.F / (4.0 * abs(dot(wo, mfSample.wm)));

            sample.reflectance = specularReflectance * D * G * fresnelData.F / (4.0 * cosThetaTS(wo) * cosThetaTS(sample.wi));

            return sample;
        }
        else
        {
            sample.wi = refractTS(wo, fresnelData.cosThetaT, fresnelData.etaTI);

            float sqrtDenom = dot(sample.wi, mfSample.wm) + dot(wo, mfSample.wm) / eta;
            float dwmDwi = abs(dot(wo, mfSample.wm)) / (sqrtDenom * sqrtDenom);

            sample.pdf = mfSample.pdf * (1.0 - fresnelData.F) * dwmDwi;

            sample.reflectance = specularTransmittance * D * G * (1.0 - fresnelData.F) * abs(fresnelData.etaTI * fresnelData.etaTI * dwmDwi);

            return sample;
        }
    }

    // FIXME: All three functions "spread blackness" :D FIrst fix evaluate and pdf - we should be able to construct sample from them
    float3 evaluate(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        bool isReflection = cosThetaI * cosThetaO > 0.0;

        float eta = interiorIor / exteriorIor;
        float etaP = isReflection ? 1.0 : (cosThetaO > 0.0 ? eta : 1.0 / eta);

        float3 wm = normalize(etaP * wi + wo);
        wm *= sign(cosThetaTS(wm));

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float D = distr.evaluate(wm);

        if(D == 0.0)
        {
            return float3(0.0);
        }


        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wo, wm), eta);

        const float G = distr.smithG1(wo, wm);

        if (isReflection)
        {
            return specularReflectance * D * G * fresnelData.F / (4.0 * cosThetaO * cosThetaI);
        }
        else
        {
            float sqrtDenom = dot(wi, wm) + dot(wo, wm) / eta;
            float dwmDwi = abs(dot(wi, wm) * dot(wo, wm) / (sqrtDenom * sqrtDenom));

            return specularTransmittance * D * G * fresnelData.etaTI * fresnelData.etaTI * (1.0 - fresnelData.F) * dwmDwi;
        }
    }

    float pdf(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        bool isReflection = cosThetaI * cosThetaO > 0.0;

        float eta = interiorIor / exteriorIor;
        float etaP = isReflection ? 1.0 : (cosThetaO > 0.0 ? eta : 1.0 / eta);

        float3 wm = normalize(etaP * wi + wo);
        wm *= sign(cosThetaTS(wm));

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wo, wm), eta);

        if (isReflection)
        {
            return distr.pdf(sign(cosThetaO) * wo, wm) * fresnelData.F / (4.0 * abs(cosThetaO));
        }
        else
        {
            float sqrtDenom = dot(wi, wm) + dot(wo, wm) / eta;
            float dwmDwi = abs(dot(wo, wm)) / (sqrtDenom * sqrtDenom);

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
        if (this.alphaTexture >= 0)
        {
            float2 alpha = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            bsdf.alphaU = alpha.x;
            bsdf.alphaV = alpha.y;
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
                    return RoughDielectricMaterial(this);
                }
            default:
                {
                    return DiffuseMaterial(this);
                }
        }
    }
}