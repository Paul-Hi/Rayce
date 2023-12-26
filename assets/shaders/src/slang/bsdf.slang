import globals;
import utils;
import random;
import data;

struct DielectricFresnelData
{
    float F;
    float cosThetaI;
    float cosThetaT;
    float etaIT;
    float etaTI;
};

float3 cosineHemisphereSample(const float2 uv = Random::rand2())
{
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

// Simliar to https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/#more-1921
float3 FresnelConductorExact(const float cosThetaI, const float3 eta, const float3 k)
{
    float cos2ThetaI = cosThetaI * cosThetaI;
    float sin2ThetaI = 1.0 - cos2ThetaI;
    float sin4ThetaI = sin2ThetaI * sin2ThetaI;

    float3 t0 = eta * eta - k * k - sin2ThetaI;
    float3 a2pb2 = safe_sqrt(t0 * t0 + 4.0 * k * k * eta * eta);
    float3 a     = safe_sqrt(0.5 * (a2pb2 + t0));

    float3 term1 = a2pb2 + cos2ThetaI;
    float3 term2 = 2.0 * a * cosThetaI;

    float3 Rs2 = (term1 - term2) / (term1 + term2);

    float3 term3 = a2pb2 * cos2ThetaI + sin4ThetaI;
    float3 term4 = term2 * sin2ThetaI;

    float3 Rp2 = Rs2 * (term3 - term4) / (term3 + term4);

    return 0.5 * (Rp2 + Rs2);
} // FIXME: Exact introduces fireflies :/

float3 FresnelConductorApprox(const float cosThetaI, const float3 eta, const float3 k)
{
    float cos2ThetaI = cosThetaI * cosThetaI;
    float3 twoEtaCos2ThetaI = 2.0 * eta * cos2ThetaI;

    float3 t0 = eta * eta + k * k;
    float3 t1 = t0 * cos2ThetaI;
    float3 Rs = (t0 - twoEtaCos2ThetaI + cos2ThetaI) / (t0 + twoEtaCos2ThetaI + cos2ThetaI);
    float3 Rp = (t1 - twoEtaCos2ThetaI + 1.0) / (t1 + twoEtaCos2ThetaI + 1.0);

    return 0.5* (Rp + Rs);
}

float fresnelDiffuseReflectance(float eta)
{
    /* Fast mode: the following code approximates the
    * diffuse Frensel reflectance for the eta<1 and
    * eta>1 cases. An evalution of the accuracy led
    * to the following scheme, which cherry-picks
    * fits from two papers where they are best.
    */
    if (eta < 1)
    {
        /* Fit by Egan and Hilgeman (1973). Works
            reasonably well for "normal" IOR values (<2).

            Max rel. error in 1.0 - 1.5 : 0.1%
            Max rel. error in 1.5 - 2   : 0.6%
            Max rel. error in 2.0 - 5   : 9.5%
        */
        return -1.4399 * (eta * eta)
                + 0.7099 * eta
                + 0.6681
                + 0.0636 / eta;
    }
    else
    {
        /* Fit by d'Eon and Irving (2011)
        *
        * Maintains a good accuracy even for
        * unrealistic IOR values.
        *
        * Max rel. error in 1.0 - 2.0   : 0.1%
        * Max rel. error in 2.0 - 10.0  : 0.2%
        */
        float invEta = 1.0 / eta,
                invEta2 = invEta * invEta,
                invEta3 = invEta2 * invEta,
                invEta4 = invEta3 * invEta,
                invEta5 = invEta4 * invEta;

        return 0.919317 - 3.4793 * invEta
                + 6.75335 * invEta2
                - 7.80989 * invEta3
                + 4.98554 * invEta4
                - 1.36881 * invEta5;
    }
}

interface IBxDF
{
    EBxDFType getType();

    BxDFSample sample(const float3 wo);
    float3 evaluate(const float3 wo, const float3 wi);
    float pdf(const float3 wo, const float3 wi);

    bool isDirac();

};

interface IBxDFAdapter
{
    EBxDFType getType();

    BxDFSample sample(const float3 wo);
    float3 evaluate(const float3 wo, const float3 wi);
    float pdf(const float3 wo, const float3 wi);

    bool isDirac();
};

struct DefaultAdapter : IBxDFAdapter
{
    __init(IBxDF bsdf)
    {
        this.bsdf0 = bsdf;
    }

    IBxDF bsdf0;

    BxDFSample sample(const float3 wo)
    {
        return bsdf0.sample(wo);
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        return bsdf0.evaluate(wo, wi);
    }

    float pdf(const float3 wo, const float3 wi)
    {
        return bsdf0.pdf(wo, wi);
    }

    bool isDirac()
    {
        return bsdf0.isDirac();
    };

    EBxDFType getType()
    {
        return bsdf0.getType();
    }
};


struct TwoSidedAdapter : IBxDFAdapter
{
    __init(IBxDF bsdf)
    {
        this.bsdf0 = bsdf;
        this.bsdf1 = bsdf;
    }

    IBxDF bsdf0;
    IBxDF bsdf1;

    BxDFSample sample(const float3 wo)
    {
        bool flipped = false;
        float3 twoSidedWo = wo;
        if (cosThetaTS(twoSidedWo) < 0.0)
        {
            twoSidedWo.z *= -1;
            flipped = true;
        }

        BxDFSample result = flipped ? bsdf1.sample(twoSidedWo) : bsdf0.sample(twoSidedWo);

        if (flipped)
        {
            result.wi.z *= -1;
        }

        return result;
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        float3 twoSidedWi = wi;
        float3 twoSidedWo = wo;
        if (cosThetaTS(wi) > 0.0)
        {
            return bsdf0.evaluate(wo, wi);
        }
        else
        {
            twoSidedWi.z *= -1;
            twoSidedWo.z *= -1;
            return bsdf1.evaluate(twoSidedWo, twoSidedWi);
        }
    }

    float pdf(const float3 wo, const float3 wi)
    {
        float3 twoSidedWi = wi;
        float3 twoSidedWo = wo;
        if (cosThetaTS(wi) > 0.0)
        {
            return bsdf0.pdf(wo, wi);
        }
        else
        {
            twoSidedWi.z *= -1;
            twoSidedWo.z *= -1;
            return bsdf1.pdf(twoSidedWo, twoSidedWi);
        }
    }

    bool isDirac()
    {
        return bsdf0.isDirac();
    };

    EBxDFType getType()
    {
        return bsdf0.getType();
    }
};

struct DiffuseBSDF : IBxDF
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
            sample.wi = -wo;
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

    // based on Sampling Visible GGX Normals with Spherical Caps https://arxiv.org/pdf/2306.05044.pdf
    MicrofacetSample sample(const float3 wo)
    {
        // sample visible
        MicrofacetSample sample;

        float2 rand = Random::rand2();

        float3 stretchedWo = normalize(float3(alphaU * wo.x, alphaV * wo.y, wo.z));

        float phi = TWO_PI * rand.x;
        float z = 1.0 - (rand.y * (1.0 + stretchedWo.z));
        float sinTheta = sqrt(clamp(1.0 - z * z, 0.0, 1.0));
        float x = sinTheta * cos(phi);
        float y = sinTheta * sin(phi);
        float3 c = float3(x, y, z);

        float3 h = c + stretchedWo;

        sample.wm = normalize(float3(alphaU * h.x, alphaV * h.y, max(0.0, h.z)));

        sample.pdf = pdf(wo, sample.wm);

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
        sample.dirac = isDirac();
        sample.wi = float3(0.0);
        sample.reflectance = float3(0.0);
        sample.pdf = 0.0;

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float cosThetaI = cosThetaTS(wo);
        const MicrofacetSample mfSample = distr.sample(sign(cosThetaI) * wo);
        if (mfSample.pdf == 0.0)
        {
            return sample;
        }

        const float D = distr.evaluate(mfSample.wm);

        if(D == 0.0)
        {
            return sample;
        }

        const float eta = interiorIor / exteriorIor;
        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wo, mfSample.wm), eta);

        if (Random::rand() <= fresnelData.F)
        {
            sample.wi = reflect(wo, mfSample.wm);

            sample.pdf = mfSample.pdf * fresnelData.F / (4.0 * abs(dot(wo, mfSample.wm)));

            const float G = distr.G(sample.wi, wo);
            sample.reflectance = specularReflectance * D * G * fresnelData.F / abs(4.0 * cosThetaTS(sample.wi) * cosThetaI);

            return sample;
        }
        else
        {
            sample.wi = refract(wo, mfSample.wm, fresnelData.cosThetaT, fresnelData.etaTI);

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

struct SmoothConductorBSDF : IBxDF
{
    BxDFSample sample(const float3 wo)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.dirac = isDirac();
        sample.wi = float3(0.0);
        sample.reflectance = float3(0.0);
        sample.pdf = 0.0;

        const float cosThetaI = cosThetaTS(wo);

        if(cosThetaI <= 0.0)
        {
            return sample;
        }

        sample.wi = reflectTS(wo);
        sample.pdf = 1.0;

        const float absNDotL = abs(cosThetaTS(sample.wi));
        sample.reflectance = absNDotL > 0.0 ? FresnelConductorApprox(cosThetaI, eta, k) * specularReflectance / absNDotL : float3(0.0);

        return sample;
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
        return EBxDFType::smoothConductor;
    }

    float3 eta, k;
    float3 specularReflectance;
};

struct RoughConductorBSDF : IBxDF
{
    BxDFSample sample(const float3 wo)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.wi = float3(0.0);
        sample.reflectance = float3(0.0);
        sample.pdf = 0.0;

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float cosThetaI = cosThetaTS(wo);

        if(cosThetaI < 0.0)
        {
            return sample;
        }

        const MicrofacetSample mfSample = distr.sample(wo);
        if (mfSample.pdf == 0.0)
        {
            return sample;
        }

        const float D = distr.evaluate(mfSample.wm);

        if(D == 0.0)
        {
            return sample;
        }

        const float3 F = FresnelConductorApprox(dot(wo, mfSample.wm), eta, k);

        sample.wi = reflect(wo, mfSample.wm);

        sample.pdf = mfSample.pdf / (4.0 * abs(dot(wo, mfSample.wm)));

        const float G = distr.G(sample.wi, wo);
        sample.reflectance = specularReflectance * D * G * F / (4.0 * abs(cosThetaTS(sample.wi)) * cosThetaI);

        return sample;
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI <= 0.0 || cosThetaO <= 0.0)
        {
            return float3(0.0);
        }

        float3 wm = wi + wo;

        if(length(wm) * length(wm) < EPSILON)
        {
            return float3(0.0);
        }

        wm = normalize(wm);

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float D = distr.evaluate(wm);

        if(D == 0.0)
        {
            return float3(0.0);
        }

        const float3 F = FresnelConductorApprox(dot(wo, wm), eta, k);

        const float G = distr.G(wi, wo);

        return specularReflectance * D * G * F / (4.0 * abs(cosThetaO) * abs(cosThetaI));
    }

    float pdf(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI <= 0.0 || cosThetaO <= 0.0)
        {
            return 0.0;
        }

        float3 wm = wi + wo;

        if(length(wm) * length(wm) < EPSILON)
        {
            return 0.0;
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        return distr.pdf(wo, wm) / abs(4.0 * abs(dot(wo, wm)));
    }

    bool isDirac()
    {
        return false;
    };

    EBxDFType getType()
    {
        return EBxDFType::roughConductor;
    }

    float3 eta, k;
    float3 specularReflectance;

    float alphaU, alphaV;
    // bool sampleVisible = true; // FIXME: Add option?
    // Distribution distribution = Distribution::GGX; // FIXME: Add option?
};

// Mitsubas plastic implementation
struct SmoothPlasticBSDF : IBxDF
{
    BxDFSample sample(const float3 wo)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.dirac = isDirac();
        sample.wi = float3(0.0);
        sample.reflectance = float3(0.0);
        sample.pdf = 0.0;

        const float cosThetaI = cosThetaTS(wo);

        if(cosThetaI <= 0.0)
        {
            return sample;
        }

        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaI, eta);

        float specularProbability = (fresnelData.F * specularSamplingWeight) /
                        (fresnelData.F * specularSamplingWeight + (1.0 - fresnelData.F) * (1.0 - specularSamplingWeight));

        if(Random::rand() <= specularProbability)
        {
            sample.wi = reflectTS(wo);
            sample.pdf = specularProbability;

            const float absNDotL = abs(cosThetaTS(sample.wi));
            sample.reflectance = absNDotL > 0.0 ? specularReflectance * fresnelData.F / absNDotL : float3(0.0);

            return sample;
        }
        else
        {
            float2 biasedSample = Random::rand2();
            biasedSample.x = (biasedSample.x - specularProbability) / (1.0 - specularProbability);
            sample.wi = cosineHemisphereSample(biasedSample);

            DielectricFresnelData fresnelOutwards = FresnelDielectric(cosThetaTS(sample.wi), eta);

            float3 diffuse = diffuseReflectance;
            if (nonlinear)
                diffuse /= (float3(1.0) - diffuse * interiorFresnelDiffuseReflection);
            else
                diffuse /= (1.0 - interiorFresnelDiffuseReflection);

            sample.pdf = (1.0 - specularProbability) * cosineHemispherePdf(sample.wi);

            const float absNDotL = abs(cosThetaTS(sample.wi));
            diffuse *= (1.0 / (eta * eta) * (1.0 - fresnelData.F) * (1.0 - fresnelOutwards.F));
            sample.reflectance = absNDotL > 0.0 ? diffuse * cosineHemispherePdf(sample.wi) / absNDotL : float3(0.0);
        }

        return sample;
    }

    float3 evaluate(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI <= 0.0 || cosThetaO <= 0.0)
        {
            return 0.0;
        }

        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaI, eta);

        if(abs(dot(reflectTS(wo), wi) - 1.0) < EPSILON) // perfect reflection?
        {
            return specularReflectance * fresnelData.F;
        }
        else
        {
            DielectricFresnelData fresnelOutwards = FresnelDielectric(cosThetaO, eta);

            float3 diffuse = diffuseReflectance;
            if (nonlinear)
                diffuse /= (float3(1.0) - diffuse * interiorFresnelDiffuseReflection);
            else
                diffuse /= (1.0 - interiorFresnelDiffuseReflection);

            diffuse *= (1.0 / (eta * eta) * (1.0 - fresnelData.F) * (1.0 - fresnelOutwards.F));
            return diffuse * cosineHemispherePdf(wo);
        }
    }

    float pdf(const float3 wo, const float3 wi)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI <= 0.0 || cosThetaO <= 0.0)
        {
            return 0.0;
        }

        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaI, eta);

        float specularProbability = (fresnelData.F * specularSamplingWeight) /
                        (fresnelData.F * specularSamplingWeight + (1.0 - fresnelData.F) * (1.0 - specularSamplingWeight));

        if(abs(dot(reflectTS(wo), wi) - 1.0) < EPSILON) // perfect reflection?
        {
            return specularProbability;
        }
        else
        {
            return (1.0 - specularProbability) * cosineHemispherePdf(wo);
        }
    }

    bool isDirac()
    {
        return false;
    };

    EBxDFType getType()
    {
        return EBxDFType::smoothConductor;
    }

    float eta;
    float interiorFresnelDiffuseReflection,
            exteriorFresnelDiffuseReflection;
    float3 diffuseReflectance;
    float3 specularReflectance;

    float specularSamplingWeight;

    bool nonlinear = false;
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

    typedef DiffuseBSDF RealBxDF;

    DiffuseBSDF createBxDF(const float2 uv)
    {
        DiffuseBSDF bsdf;
        bsdf.diffuseReflectance = this.diffuseReflectance;
        if (this.diffuseReflectanceTexture >= 0)
        {
            bsdf.diffuseReflectance = gTextures[NonUniformResourceIndex(this.diffuseReflectanceTexture)].Sample(uv).rgb;
        }

        return bsdf;
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
        this.alpha = materialData.alpha.xy;
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

    typedef SmoothConductorBSDF RealBxDF;

    SmoothConductorBSDF createBxDF(const float2 uv)
    {
        SmoothConductorBSDF bsdf;
        bsdf.specularReflectance = this.specularReflectance;
        if (this.specularReflectanceTexture >= 0)
        {
            bsdf.specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        bsdf.eta = this.eta;
        bsdf.k = this.k;
        if (this.etaTexture >= 0)
        {
            bsdf.eta = gTextures[NonUniformResourceIndex(this.etaTexture)].Sample(uv).rgb;
        }
        if (this.kTexture >= 0)
        {
            bsdf.k = gTextures[NonUniformResourceIndex(this.kTexture)].Sample(uv).rgb;
        }

        bsdf.eta = max(bsdf.eta, float3(EPSILON)); // FIXME: This could be handled better
        bsdf.k = max(bsdf.k, float3(EPSILON)); // FIXME: This could be handled better

        return bsdf;
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

    typedef RoughConductorBSDF RealBxDF;

    RoughConductorBSDF createBxDF(const float2 uv)
    {
        RoughConductorBSDF bsdf;
        bsdf.specularReflectance = this.specularReflectance;
        if (this.specularReflectanceTexture >= 0)
        {
            bsdf.specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        bsdf.alphaU = alpha.x;
        bsdf.alphaV = alpha.y;
        if (this.alphaTexture >= 0)
        {
            float2 alphaT = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            bsdf.alphaU = alphaT.x;
            bsdf.alphaV = alphaT.y;
        }

        bsdf.eta = this.eta;
        bsdf.k = this.k;
        if (this.etaTexture >= 0)
        {
            bsdf.eta = gTextures[NonUniformResourceIndex(this.etaTexture)].Sample(uv).rgb;
        }
        if (this.kTexture >= 0)
        {
            bsdf.k = gTextures[NonUniformResourceIndex(this.kTexture)].Sample(uv).rgb;
        }

        bsdf.eta = max(bsdf.eta, float3(EPSILON)); // FIXME: This could be handled better
        bsdf.k = max(bsdf.k, float3(EPSILON)); // FIXME: This could be handled better

        bsdf.alphaU = max(bsdf.alphaU, EPSILON); // FIXME: This could be handled better
        bsdf.alphaV = max(bsdf.alphaV, EPSILON); // FIXME: This could be handled better

        return bsdf;
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

    typedef SmoothPlasticBSDF RealBxDF;

    SmoothPlasticBSDF createBxDF(const float2 uv)
    {
        SmoothPlasticBSDF bsdf;
        bsdf.diffuseReflectance = this.diffuseReflectance;
        bsdf.specularReflectance = this.specularReflectance;
        if (this.diffuseReflectanceTexture >= 0)
        {
            bsdf.diffuseReflectance = gTextures[NonUniformResourceIndex(this.diffuseReflectanceTexture)].Sample(uv).rgb;
        }
        if (this.specularReflectanceTexture >= 0)
        {
            bsdf.specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        bsdf.eta = this.eta;
        bsdf.interiorFresnelDiffuseReflection = fresnelDiffuseReflectance(1.0 / this.eta);
        bsdf.exteriorFresnelDiffuseReflection = fresnelDiffuseReflectance(this.eta);

        float diffuseAvg = averageLuminance(bsdf.diffuseReflectance);
        float specularAvg = averageLuminance(bsdf.specularReflectance);

        bsdf.specularSamplingWeight = specularAvg / (diffuseAvg + specularAvg);

        return bsdf;
    }

    float eta;
    float3 diffuseReflectance;
    int diffuseReflectanceTexture;
    float3 specularReflectance;
    int specularReflectanceTexture;
};
extension Material
{
    IMaterial getMaterial()
    {
        switch(this.bxdfType)
        {
            case EBxDFType::diffuse:
                {
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
                    bool rough = (length(this.alpha) > EPSILON) || this.alphaTexture >= 0;
                    if(!rough)
                    {
                        return SmoothDielectricMaterial(this);
                    }

                    return RoughDielectricMaterial(this);
                }
            case EBxDFType::smoothConductor:
                {
                    return SmoothConductorMaterial(this);
                }
            case EBxDFType::roughConductor:
                {
                    bool rough = (length(this.alpha) > EPSILON) || this.alphaTexture >= 0;
                    if(!rough)
                    {
                        return SmoothConductorMaterial(this);
                    }

                    return RoughConductorMaterial(this);
                }
            case EBxDFType::smoothPlastic:
                {
                    return SmoothPlasticMaterial(this);
                }
            default:
                {
                    return DiffuseMaterial(this);
                }
        }
    }
}

extension IMaterial
{
    IBxDFAdapter getAdapter(const IBxDF bsdf, bool twoSided)
    {
        if(twoSided == 1)
        {
            return TwoSidedAdapter(bsdf);
        }

        return DefaultAdapter(bsdf);
    }
}