import globals;
import utils;
import random;
import data;

float2 sampleUniformConcentricDisk(const float2 uv)
{
    float2 mapped = 2.0 * uv - float2(1.0);
    if (mapped.x == 0.0 && mapped.y == 0.0)
    {
        return float2(0.0);
    }

    // concentric mapping
    float theta, r;
    if (abs(mapped.x) > abs(mapped.y))
    {
        r = mapped.x;
        theta = QUARTER_PI * (mapped.y / mapped.x);
    }
    else
    {
        r = mapped.y;
        theta = HALF_PI - QUARTER_PI * (mapped.x / mapped.y);
    }

    return r * float2(cos(theta), sin(theta));

}

float3 cosineHemisphereSample(const float2 uv = Random::rand2())
{
    float2 d = sampleUniformConcentricDisk(uv);

    float z = safe_sqrt(1.0 - sqr(d.x) - sqr(d.y));

    return float3(d.x, d.y, z);
}

float cosineHemispherePdf(const float cosTheta)
{
    return cosTheta * INV_PI;
}

float fresnelDielectric(float cosThetaI, float eta)
{
    if(eta == 1.0)
    {
        return 0.0;
    }

    if (cosThetaI < 0.0)
    {
        eta = 1.0 / eta;
        cosThetaI *= -1.0;
    }

    float cosThetaTSqrd = negativeMultiplyAndAdd(negativeMultiplyAndAdd(cosThetaI, cosThetaI, 1.0), eta * eta, 1.0);

    // total internal reflection
    if (cosThetaTSqrd <= 0.0)
    {
        return 1.0;
    }

    float cosThetaT = sqrt(cosThetaTSqrd);

    float Rs = negativeMultiplyAndAdd(eta, cosThetaT, cosThetaI) / multiplyAndAdd(eta, cosThetaT, cosThetaI);
    float Rp = multiplyAndAdd(eta, cosThetaI, -cosThetaT) / multiplyAndAdd(eta, cosThetaI, cosThetaT);

    return 0.5 * (sqr(Rs) + sqr(Rp));
}

// Simliar to https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/#more-1921
float3 fresnelConductorExact(const float cosThetaI, const float3 eta, const float3 k)
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
}

float3 fresnelConductorApprox(const float cosThetaI, const float3 eta, const float3 k)
{
    float cos2ThetaI = cosThetaI * cosThetaI;
    float3 twoEtaCos2ThetaI = 2.0 * eta * cos2ThetaI;

    float3 t0 = eta * eta + k * k;
    float3 t1 = t0 * cos2ThetaI;
    float3 Rs = (t0 - twoEtaCos2ThetaI + cos2ThetaI) / (t0 + twoEtaCos2ThetaI + cos2ThetaI);
    float3 Rp = (t1 - twoEtaCos2ThetaI + 1.0) / (t1 + twoEtaCos2ThetaI + 1.0);

    return 0.5 * (Rp + Rs);
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

struct GGXDistribution
{
    __init(float alphaU, float alphaV)
    {
        this.alphaU = alphaU;
        this.alphaV = alphaV;
    }

    // VNDF
    float D(const float3 wm)
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

    float D(const float3 w, const float3 wm)
    {
        return G1(w) / abs(cosThetaTS(w)) * abs(dot(w, wm)) * D(wm);
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
    Optional<MicrofacetSample> sample(const float3 wo)
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

        if(sample.pdf == 0.0)
        {
            return none;
        }

        return sample;
    }

    float G1(const float3 w)
    {
        return 1.0 / (1.0 + lambda(w));
    }

    float G(const float3 wo, const float3 wi)
    {
        return 1.0 / (1.0 + lambda(wi) + lambda(wo));
    }

    float pdf(const float3 w, const float3 wm)
    {
        return D(w, wm);
    }

    bool effectivelySmooth()
    {
        return max(alphaU, alphaV) < EPSILON;
    }

    float alphaU, alphaV;
};

interface IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode = ETransportMode::radiance);
    float3 f(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);
    float pdf(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);

    EBxDFFlags flags();
};

interface IBxDFAdapter
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode = ETransportMode::radiance);
    float3 f(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);
    float pdf(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);

    EBxDFFlags flags();
};

struct DefaultAdapter : IBxDFAdapter
{
    __init(IBxDF bsdf)
    {
        this.bsdf0 = bsdf;
    }

    IBxDF bsdf0;

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        return bsdf0.sample(wo, mode);
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return bsdf0.f(wo, wi, mode);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return bsdf0.pdf(wo, wi, mode);
    }

    EBxDFFlags flags()
    {
        return bsdf0.flags();
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

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        bool flipped = false;
        float3 twoSidedWo = wo;
        if (cosThetaTS(twoSidedWo) < 0.0)
        {
            twoSidedWo.z *= -1;
            flipped = true;
        }

        Optional<BxDFSample> result = flipped ? bsdf1.sample(twoSidedWo, mode) : bsdf0.sample(twoSidedWo, mode);

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

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        float3 twoSidedWo = wo;
        float3 twoSidedWi = wi;
        if (cosThetaTS(wo) > 0.0)
        {
            return bsdf0.f(wo, wi, mode);
        }
        else
        {
            twoSidedWo.z *= -1;
            twoSidedWi.z *= -1;
            return bsdf1.f(twoSidedWo, twoSidedWi, mode);
        }
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        float3 twoSidedWo = wo;
        float3 twoSidedWi = wi;
        if (cosThetaTS(wo) > 0.0)
        {
            return bsdf0.pdf(wo, wi, mode);
        }
        else
        {
            twoSidedWo.z *= -1;
            twoSidedWi.z *= -1;
            return bsdf1.pdf(twoSidedWo, twoSidedWi, mode);
        }
    }

    EBxDFFlags flags()
    {
        return bsdf0.flags(); // FIXME
    }
};

bool sameHemisphere(const float3 w, const float3 wp)
{
    const float cosTheta = cosThetaTS(w);
    const float cosThetaP = cosThetaTS(wp);

    return cosTheta * cosThetaP > 0.0;
}

bool sameHemisphere(const float cosTheta, const float cosThetaP)
{
    return cosTheta * cosThetaP > 0.0;
}

struct DiffuseBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;

        sample.flags = EBxDFFlags::diffuseReflection;
        sample.wi = cosineHemisphereSample();

        if (cosThetaTS(wo) < 0.0)
        {
            sample.wi.z *= -1.0;
        }

        sample.pdf = cosineHemispherePdf(abs(cosThetaTS(sample.wi)));

        sample.f = diffuseReflectance * INV_PI;

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(!sameHemisphere(wo, wi))
        {
            return float3(0.0);
        }

        return diffuseReflectance * INV_PI;
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(!sameHemisphere(wo, wi))
        {
            return 0.0;
        }

        return cosineHemispherePdf(abs(cosThetaTS(wi)));
    }

    EBxDFFlags flags()
    {
        return length(diffuseReflectance) > 0.0 ? EBxDFFlags::diffuseReflection : EBxDFFlags::unset;
    }

    float3 diffuseReflectance;
};

struct ConductorBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;

        if(distr.effectivelySmooth())
        {
            sample.flags = EBxDFFlags::specularReflection;
            sample.wi = reflectTS(wo);
            sample.pdf = 1.0;

            const float absCosThetaI = abs(cosThetaTS(sample.wi));

            sample.f = fresnelConductorExact(absCosThetaI, eta, k) * specularReflectance / absCosThetaI;

            return sample;
        }
        else
        {
            sample.flags = EBxDFFlags::glossyReflection;

            const Optional<MicrofacetSample> mfSample = distr.sample(wo);

            if (mfSample == none)
            {
                return none;
            }

            sample.wi = reflect(wo, mfSample.value.wm);

            const float cosThetaI = cosThetaTS(sample.wi);
            const float cosThetaO = cosThetaTS(wo);

            if(!sameHemisphere(cosThetaI, cosThetaO))
            {
                return none;
            }

            const float absCosThetaOM = abs(dot(wo, mfSample.value.wm));

            sample.pdf = mfSample.value.pdf / (4.0 * absCosThetaOM);

            const float3 F = fresnelConductorExact(absCosThetaOM, eta, k);

            const float D = distr.D(mfSample.value.wm);
            const float G = distr.G(wo, sample.wi);
            sample.f = D * G * F * specularReflectance / (4.0 * abs(cosThetaI) * abs(cosThetaO));

            return sample;
        }

        return none;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(distr.effectivelySmooth())
        {
            return float3(0.0);
        }

        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(!sameHemisphere(cosThetaI, cosThetaO))
        {
            return float3(0.0);
        }

        const float absCosThetaI = abs(cosThetaI);
        const float absCosThetaO = abs(cosThetaO);

        if(absCosThetaI == 0.0 || absCosThetaO == 0.0)
        {
            return float3(0.0);
        }

        float3 wm = wi + wo;

        if(dot(wm, wm) == 0.0)
        {
            return float3(0.0);
        }

        wm = normalize(wm);

        const float D = distr.D(wm);

        if(D == 0.0)
        {
            return float3(0.0);
        }

        const float3 F = fresnelConductorExact(abs(dot(wo, wm)), eta, k);

        const float G = distr.G(wo, wi);

        return D * G * F * specularReflectance / (4.0 * absCosThetaI * absCosThetaO);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(distr.effectivelySmooth())
        {
            return 0.0;
        }

        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(!sameHemisphere(cosThetaI, cosThetaO))
        {
            return 0.0;
        }

        const float absCosThetaI = abs(cosThetaI);
        const float absCosThetaO = abs(cosThetaO);

        if(absCosThetaI == 0.0 || absCosThetaO == 0.0)
        {
            return 0.0;
        }

        float3 wm = wi + wo;

        if(dot(wm, wm) == 0.0)
        {
            return 0.0;
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        return distr.pdf(wo, wm) / (4.0 * abs(dot(wo, wm)));
    }

    EBxDFFlags flags()
    {
        return distr.effectivelySmooth() ? EBxDFFlags::specularReflection : EBxDFFlags::glossyReflection;
    }

    float3 eta, k;
    float3 specularReflectance;

    GGXDistribution distr;

    // bool sampleVisible = true; // FIXME: Add option?
    // Distribution distribution = Distribution::GGX; // FIXME: Add option?
};

/*
struct SmoothDielectricBSDF : IBxDF
{
    BxDFSample sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaTS(wi), eta);

        if (Random::rand() <= fresnelData.F)
        {
            sample.wo = reflectTS(wi);
            sample.pdf = fresnelData.F;

            sample.f = specularReflectance;

            return sample;
        }
        else
        {
            sample.wo = refractTS(wi, fresnelData.cosThetaT, fresnelData.etaTI);
            sample.pdf = 1.0 - fresnelData.F;

            sample.f = specularTransmittance / (fresnelData.etaIT * fresnelData.etaIT);

            return sample;
        }
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaTS(wi), eta);

        if(cosThetaTS(wi) * cosThetaTS(wo) >= 0.0) // reflection
        {
            if(abs(dot(reflectTS(wi), wo) - 1.0) > EPSILON) // no perfect reflection
            {
                return float3(0.0);
            }

            return specularReflectance * fresnelData.F;
        }
        else
        {
            if(abs(dot(refractTS(wi, fresnelData.cosThetaT, fresnelData.etaTI), wo) - 1.0) > EPSILON) // no perfect transmission
            {
                return float3(0.0);
            }

            return specularTransmittance * (1.0 - fresnelData.F) / (fresnelData.etaIT * fresnelData.etaIT);
        }
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaTS(wi), eta);

        if(cosThetaTS(wi) * cosThetaTS(wo) >= 0.0) // reflection
        {
            if(abs(dot(reflectTS(wi), wo) - 1.0) > EPSILON) // no perfect reflection
    {
        return 0.0;
            }

            return fresnelData.F;
        }
        else
        {
            if(abs(dot(refractTS(wi, fresnelData.cosThetaT, fresnelData.etaTI), wo) - 1.0) > EPSILON) // no perfect transmission
            {
                return 0.0;
            }

            return 1.0 - fresnelData.F;
        }
    }

    bool isDirac()
    {
        return true;
    };

    EBxDFType getType()
    {
        return EBxDFType::smoothDielectric;
    }

    float eta;
    float3 specularReflectance;
    float3 specularTransmittance;
};

struct SmoothThinDielectricBSDF : IBxDF
{
    BxDFSample sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        DielectricFresnelData fresnelData = FresnelDielectric(cosThetaTS(wi), eta);

        if(fresnelData.F < 1.0)
        {
        // Account for internal reflections
        fresnelData.F *= 2.0 / (1.0 + fresnelData.F);
        }

        if (Random::rand() <= fresnelData.F)
        {
            sample.wo = reflectTS(wi);
            sample.pdf = fresnelData.F;

            sample.f = specularReflectance;

            return sample;
        }
        else
        {
            sample.wo = refractTS(wi, fresnelData.cosThetaT, fresnelData.etaTI);
            sample.pdf = 1.0 - fresnelData.F;

            sample.f = specularTransmittance;

            return sample;
        }
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        DielectricFresnelData fresnelData = FresnelDielectric(cosThetaTS(wi), eta);

        if(fresnelData.F < 1.0)
        {
            // Account for internal reflections
            fresnelData.F *= 2.0 / (1.0 + fresnelData.F);
        }

        if(cosThetaTS(wi) * cosThetaTS(wo) >= 0.0) // reflection
        {
            if(abs(dot(reflectTS(wi), wo) - 1.0) > EPSILON) // no perfect reflection
    {
        return float3(0.0);
    }

            return specularReflectance * fresnelData.F;
        }
        else
        {
            if(abs(dot(refractTS(wi, fresnelData.cosThetaT, fresnelData.etaTI), wo) - 1.0) > EPSILON) // no perfect transmission
            {
                return float3(0.0);
            }

            return specularTransmittance * (1.0 - fresnelData.F);
        }
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaTS(wi), eta);

        if(cosThetaTS(wi) * cosThetaTS(wo) >= 0.0) // reflection
        {
            if(abs(dot(reflectTS(wi), wo) - 1.0) > EPSILON) // no perfect reflection
    {
        return 0.0;
            }

            return fresnelData.F;
        }
        else
        {
            if(abs(dot(refractTS(wi, fresnelData.cosThetaT, fresnelData.etaTI), wo) - 1.0) > EPSILON) // no perfect transmission
            {
                return 0.0;
            }

            return 1.0 - fresnelData.F;
        }
    }

    bool isDirac()
    {
        return true;
    };

    EBxDFType getType()
    {
        return EBxDFType::smoothDielectricThin;
    }

    float eta;
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
    float f(const float3 wm)
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

    float f(const float3 w, const float3 wm)
    {
        return G1(w) / abs(cosThetaTS(w)) * abs(dot(w, wm)) * f(wm);
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
    MicrofacetSample sample(const float3 wo, const ETransportMode mode)
    {
        // sample visible
        MicrofacetSample sample;

        float2 rand = Random::rand2();

        float3 stretchedWi = normalize(float3(alphaU * wi.x, alphaV * wi.y, wi.z));

        float phi = TWO_PI * rand.x;
        float z = 1.0 - (rand.y * (1.0 + stretchedWi.z));
        float sinTheta = sqrt(clamp(1.0 - z * z, 0.0, 1.0));
        float x = sinTheta * cos(phi);
        float y = sinTheta * sin(phi);
        float3 c = float3(x, y, z);

        float3 h = c + stretchedWi;

        sample.wm = normalize(float3(alphaU * h.x, alphaV * h.y, max(0.0, h.z)));

        sample.pdf = pdf(wi, sample.wm);

        return sample;
    }

    float G1(const float3 w)
    {
        return 1.0 / (1.0 + lambda(w));
    }

    float G(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return 1.0 / (1.0 + lambda(wi) + lambda(wo));
    }

    float pdf(const float3 w, const float3 wm)
    {
        return f(w, wm);
    }

    float alphaU, alphaV;
};

struct RoughDielectricBSDF : IBxDF
{
    BxDFSample sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.wo = float3(0.0);
        sample.f = float3(0.0);
        sample.pdf = 0.0;

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float cosThetaI = cosThetaTS(wi);
        const MicrofacetSample mfSample = distr.sample(sign(cosThetaI) * wi);
        if (mfSample.pdf == 0.0)
        {
            return sample;
        }

        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wi, mfSample.wm), eta);

        if (Random::rand() <= fresnelData.F)
        {
            sample.wo = reflect(wi, mfSample.wm);

            sample.pdf = mfSample.pdf * fresnelData.F / (4.0 * abs(dot(wi, mfSample.wm)));

            const float G = distr.G(wi, sample.wo);
            sample.f = specularReflectance * G;

            return sample;
        }
        else
        {
            sample.wo = refract(wi, mfSample.wm, fresnelData.cosThetaT, fresnelData.etaTI);

            float sqrtDenom = dot(sample.wo, mfSample.wm) + dot(wi, mfSample.wm) / fresnelData.etaTI;
            float dwmDwi = abs(dot(sample.wo, mfSample.wm)) / (sqrtDenom * sqrtDenom);

            sample.pdf = abs(mfSample.pdf * (1.0 - fresnelData.F) * dwmDwi);

            const float G = distr.G(wi, sample.wo);
            sample.f = specularTransmittance * G / (fresnelData.etaIT * fresnelData.etaIT);

            return sample;
        }
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        bool isReflection = cosThetaI * cosThetaO > 0.0;

        float etaP = isReflection ? 1.0 : (cosThetaO > 0.0 ? eta : 1.0 / eta);

        float3 wm = wi + etaP * wo;

        if(length(wm) * length(wm) < EPSILON)
        {
            return float3(0.0);
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        if (dot(wi, wm) * cosThetaI <= 0.0 || dot(wo, wm) * cosThetaO < 0.0)
        {
            return float3(0.0);
        }

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float D = distr.f(wm);

        if(D == 0.0)
        {
            return float3(0.0);
        }

        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wi, wm), eta);

        const float G = distr.G(wi, wo);

        if (isReflection)
        {
            return specularReflectance * D * G * fresnelData.F / abs(4.0 * abs(cosThetaI));
        }
        else
        {
            float sqrtDenom = (dot(wo, wm) + dot(wi, wm) / etaP);
            float dwmDwi = abs(dot(wi, wm) * dot(wo, wm) / (sqrtDenom * sqrtDenom * abs(cosThetaI)));

            return specularTransmittance * D * G * (1.0 - fresnelData.F) * dwmDwi / (etaP * etaP);
        }
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        bool isReflection = cosThetaI * cosThetaO > 0.0;

        float etaP = isReflection ? 1.0 : (cosThetaO > 0.0 ? eta : 1.0 / eta);

        float3 wm = etaP * wi + wo;

        if(cosThetaO == 0.0 || cosThetaI == 0.0 || length(wm) * length(wm) < EPSILON)
        {
            return 0.0;
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        if (dot(wi, wm) * cosThetaI <= 0.0 || dot(wo, wm) * cosThetaO < 0.0)
        {
            return 0.0;
        }

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const DielectricFresnelData fresnelData = FresnelDielectric(dot(wi, wm), eta);

        if (isReflection)
        {
            return distr.pdf(sign(cosThetaI) * wi, wm) * fresnelData.F / abs(4.0 * dot(wi, wm));
        }
        else
        {
            float sqrtDenom = dot(wo, wm) + dot(wi, wm) / etaP;
            float dwmDwi = abs(dot(wo, wm)) / (sqrtDenom * sqrtDenom);

            return abs(distr.pdf(sign(cosThetaI) * wi, wm) * (1.0 - fresnelData.F) * dwmDwi);
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

    float eta;
    float3 specularReflectance;
    float3 specularTransmittance;

    float alphaU, alphaV;
    // bool sampleVisible = true; // FIXME: Add option?
    // Distribution distribution = Distribution::GGX; // FIXME: Add option?
};

struct SmoothConductorBSDF : IBxDF
{
    BxDFSample sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.dirac = isDirac();
        sample.wo = float3(0.0);
        sample.f = float3(0.0);
        sample.pdf = 0.0;

        const float cosThetaI = cosThetaTS(wi);

        if(cosThetaI <= 0.0)
        {
            return sample;
        }

        sample.wo = reflectTS(wi);
        sample.pdf = 1.0;

        sample.f = FresnelConductorApprox(cosThetaI, eta, k) * specularReflectance;

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const float cosThetaI = cosThetaTS(wi);
        if(cosThetaI <= 0.0 || cosThetaTS(wo) <= 0.0 || abs(dot(reflectTS(wi), wo) - 1.0) > EPSILON) // no perfect reflection
        {
            return float3(0.0);
        }

        return FresnelConductorApprox(cosThetaI, eta, k) * specularReflectance;
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(cosThetaTS(wi) <= 0.0 || cosThetaTS(wo) <= 0.0 || abs(dot(reflectTS(wi), wo) - 1.0) > EPSILON) // no perfect reflection
        {
            return 0.0;
        }

        return 1.0;
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
    BxDFSample sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.wo = float3(0.0);
        sample.f = float3(0.0);
        sample.pdf = 0.0;

        GGXDistribution distr = GGXDistribution(alphaU, alphaV);

        const float cosThetaI = cosThetaTS(wi);

        if(cosThetaI <= 0.0)
        {
            return sample;
        }

        const MicrofacetSample mfSample = distr.sample(wi);
        if (mfSample.pdf == 0.0)
        {
            return sample;
        }

        const float3 F = FresnelConductorApprox(dot(wi, mfSample.wm), eta, k);

        sample.wo = reflect(wi, mfSample.wm);

        sample.pdf = mfSample.pdf / (4.0 * abs(dot(wi, mfSample.wm)));

        const float G = distr.G(wi, sample.wo);
        sample.f = specularReflectance * G * F;

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
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

        const float D = distr.f(wm);

        if(D == 0.0)
        {
            return float3(0.0);
        }

        const float3 F = FresnelConductorApprox(dot(wi, wm), eta, k);

        const float G = distr.G(wi, wo);

        return specularReflectance * D * G * F / (4.0 * cosThetaI);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
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

        return distr.pdf(wi, wm) / (4.0 * abs(dot(wi, wm)));
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
    BxDFSample sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.dirac = isDirac();
        sample.wo = float3(0.0);
        sample.f = float3(0.0);
        sample.pdf = 0.0;

        const float cosThetaI = cosThetaTS(wi);

        if(cosThetaI <= 0.0)
        {
            return sample;
        }

        const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaI, eta);

        float specularProbability = (fresnelData.F * specularSamplingWeight) /
                        (fresnelData.F * specularSamplingWeight + (1.0 - fresnelData.F) * (1.0 - specularSamplingWeight));

        if(Random::rand() <= specularProbability)
        {
            sample.wo = reflectTS(wi);
            sample.pdf = specularProbability;

            sample.f = specularReflectance * fresnelData.F / specularProbability;

            return sample;
        }
        else
        {
            float2 biasedSample = Random::rand2();
            biasedSample.x = (biasedSample.x - specularProbability) / (1.0 - specularProbability);
            sample.wo = cosineHemisphereSample(biasedSample);

            DielectricFresnelData fresnelOutwards = FresnelDielectric(cosThetaTS(sample.wo), eta);

            float3 diffuse = diffuseReflectance;
            if (nonlinear)
                diffuse /= (float3(1.0) - diffuse * interiorFresnelDiffuseReflection);
            else
                diffuse /= (1.0 - interiorFresnelDiffuseReflection);

            sample.pdf = (1.0 - specularProbability) * abs(cosineHemispherePdf(sample.wo));

            diffuse *= (1.0 / (eta * eta) * (1.0 - fresnelData.F) * (1.0 - fresnelOutwards.F));
            sample.f = diffuse / (1.0 - specularProbability);
        }

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
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
            return diffuse * abs(cosineHemispherePdf(wo));
        }
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
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
            return (1.0 - specularProbability) * abs(cosineHemispherePdf(wo));
        }
    }

    bool isDirac()
    {
        return false;
    };

    EBxDFType getType()
    {
        return EBxDFType::smoothPlastic;
    }

    float eta;
    float interiorFresnelDiffuseReflection,
            exteriorFresnelDiffuseReflection;
    float3 diffuseReflectance;
    float3 specularReflectance;

    float specularSamplingWeight;

    bool nonlinear = true;
};

// Mitsubas plastic implementation
struct RoughPlasticBSDF : IBxDF
{
    BxDFSample sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;
        sample.dirac = isDirac();
        sample.dirac = isDirac();
        sample.wo = float3(0.0);
        sample.f = float3(0.0);
        sample.pdf = 0.0;

        const float cosThetaI = cosThetaTS(wi);

        if(cosThetaI <= 0.0)
        {
            return sample;
        }

        const float factor = 0.0;

        float specularProbability = (factor * specularSamplingWeight) /
                        (factor * specularSamplingWeight + (1.0 - factor) * (1.0 - specularSamplingWeight));

        if(Random::rand() <= specularProbability)
        {
            GGXDistribution distr = GGXDistribution(alpha, alpha);

            const MicrofacetSample mfSample = distr.sample(wi);
            if (mfSample.pdf == 0.0)
            {
                return sample;
            }

            sample.wo = reflect(wi, mfSample.wm);

            sample.pdf += specularProbability * mfSample.pdf / abs(4.0 * abs(dot(sample.wo, mfSample.wm)));

            const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaI, eta);

            const float G = distr.G(wi, sample.wo);

            sample.f += specularReflectance * G * fresnelData.F / specularProbability;
        }
        else
        {
            float2 biasedSample = Random::rand2();
            biasedSample.x = (biasedSample.x - specularProbability) / (1.0 - specularProbability);
            sample.wo = cosineHemisphereSample(biasedSample);

            float transmittance12 = 0.0;
            float transmittance21 = 0.0;
            float fresnelDiffuseReflection = 0.0;

            float3 diffuse = diffuseReflectance;
            if (nonlinear)
                diffuse /= (float3(1.0) - diffuse * fresnelDiffuseReflection);
            else
                diffuse /= (1.0 - fresnelDiffuseReflection);

            sample.pdf += (1.0 - specularProbability) * abs(cosineHemispherePdf(sample.wo));

            diffuse *= (1.0 / (eta * eta) * transmittance12 * transmittance21);
            sample.f += diffuse;
        }

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI <= 0.0 || cosThetaO <= 0.0)
        {
            return float3(0.0);
        }

        float3 result = float3(0.0);

        // reflection

        float3 wm = wi + wo;

        if(length(wm) * length(wm) < EPSILON)
        {
            return float3(0.0);
        }

        wm = normalize(wm);

        GGXDistribution distr = GGXDistribution(alpha, alpha);

        const float D = distr.f(wm);

        if(D != 0.0)
        {
            const DielectricFresnelData fresnelData = FresnelDielectric(cosThetaI, eta);

            const float G = distr.G(wi, wo);

            result += specularReflectance * D * G * fresnelData.F / (4.0 * abs(cosThetaO) * abs(cosThetaI));
        }

        // transmission and reflection and transmission

        float transmittance12 = 0.0;
        float transmittance21 = 0.0;
        float fresnelDiffuseReflection = 0.0;

        float3 diffuse = diffuseReflectance;
        if (nonlinear)
            diffuse /= (float3(1.0) - diffuse * fresnelDiffuseReflection);
        else
            diffuse /= (1.0 - fresnelDiffuseReflection);

        diffuse *= (1.0 / (eta * eta) * transmittance12 * transmittance21);
        result += diffuse * abs(cosineHemispherePdf(wo));

        return result;
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI <= 0.0 || cosThetaO <= 0.0)
        {
            return 0.0;
        }

        const float factor = 0.0;

        float specularProbability = (factor * specularSamplingWeight) /
                        (factor * specularSamplingWeight + (1.0 - factor) * (1.0 - specularSamplingWeight));

        float diffuseProbability = 1.0 - specularProbability;

        float result = 0.0;

        // reflection

        float3 wm = wi + wo;

        if(length(wm) * length(wm) < EPSILON)
        {
            return float(0.0);
        }

        wm = normalize(wm);

        GGXDistribution distr = GGXDistribution(alpha, alpha);

        const float G = distr.pdf(wi, wm);

        result += specularProbability * distr.pdf(wi, wm) / abs(4.0 * abs(dot(wo, wm)));

        // transmission and reflection and transmission

        result += (1.0 - specularProbability) * abs(cosineHemispherePdf(wo));

        return result;
    }

    bool isDirac()
    {
        return false;
    };

    EBxDFType getType()
    {
        return EBxDFType::roughPlastic;
    }

    float eta;
    float3 diffuseReflectance;
    float3 specularReflectance;

    float alpha;

    float specularSamplingWeight;

    bool nonlinear = true;
};
*/
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

struct ConductorMaterial : IMaterial
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

    typedef ConductorBSDF RealBxDF;

    ConductorBSDF createBxDF(const float2 uv)
    {
        ConductorBSDF bsdf;
        bsdf.specularReflectance = this.specularReflectance;
        if (this.specularReflectanceTexture >= 0)
        {
            bsdf.specularReflectance = gTextures[NonUniformResourceIndex(this.specularReflectanceTexture)].Sample(uv).rgb;
        }

        float alphaU = alpha.x;
        float alphaV = alpha.y;
        if (this.alphaTexture >= 0)
        {
            float2 alphaT = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            alphaU = alphaT.x;
            alphaV = alphaT.y;
        }

        bsdf.distr = GGXDistribution(alphaU, alphaV);

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

    float2 alpha;
    int alphaTexture;
};

/*
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

        bsdf.eta = this.interiorIor / this.exteriorIor;

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

        bsdf.eta = this.interiorIor / this.exteriorIor;

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

        bsdf.eta = this.interiorIor / this.exteriorIor;

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

    typedef RoughPlasticBSDF RealBxDF;

    RoughPlasticBSDF createBxDF(const float2 uv)
    {
        RoughPlasticBSDF bsdf;
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
        //bsdf.interiorFresnelDiffuseReflection = fresnelDiffuseReflectance(1.0 / this.eta);
        //bsdf.exteriorFresnelDiffuseReflection = fresnelDiffuseReflectance(this.eta);

        bsdf.alpha = alpha;
        if (this.alphaTexture >= 0)
        {
            float2 alphaT = gTextures[NonUniformResourceIndex(this.alphaTexture)].Sample(uv).rg;
            bsdf.alpha = max(alphaT.x, alphaT.y);
        }

        float diffuseAvg = averageLuminance(bsdf.diffuseReflectance);
        float specularAvg = averageLuminance(bsdf.specularReflectance);

        bsdf.specularSamplingWeight = specularAvg / (diffuseAvg + specularAvg);

        bsdf.alpha = max(bsdf.alpha, EPSILON); // FIXME: This could be handled better

        return bsdf;
    }

    float eta;
    float3 diffuseReflectance;
    int diffuseReflectanceTexture;
    float3 specularReflectance;
    int specularReflectanceTexture;

    float alpha; // no anisotropy
    int alphaTexture;
};
*/
extension Material
{
    [mutating]
    IMaterial getMaterial()
    {
        switch(this.bxdfType)
        {
            case EBxDFType::lambertDiffuse:
                {
                    return DiffuseMaterial(this);
                }
            case EBxDFType::smoothConductor:
                {
                    this.alpha = float3(0.0);
                }
            case EBxDFType::roughConductor:
                {
                    return ConductorMaterial(this);
                }
            case EBxDFType::smoothDielectric:
            case EBxDFType::smoothDielectricThin:
            case EBxDFType::roughDielectric:
            case EBxDFType::smoothPlastic:
            case EBxDFType::roughPlastic:
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