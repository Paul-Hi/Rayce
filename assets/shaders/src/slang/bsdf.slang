#pragma once

import globals;
import utils;
import random;
import data;

float2 sampleUniformConcentricDisk(const float2 uv)
{
    const float2 mapped = 2.0 * uv - float2(1.0);
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
    const float2 d = sampleUniformConcentricDisk(uv);

    const float z = safe_sqrt(1.0 - sqr(d.x) - sqr(d.y));

    return float3(d.x, d.y, z);
}

float cosineHemispherePdf(const float cosTheta)
{
    return cosTheta * INV_PI;
}

float fresnelDielectric(const float cosThetaI, const float etati, const float etait)
{
    float eta = etati;
    float _ignore0 = 0.0;
    float _ignore1 = 0.0;
    return fresnelDielectric(cosThetaI, eta, _ignore0, _ignore1);
}

float fresnelDielectric(const float cosThetaI, inout float etati, out float etait, out float cosThetaT)
{
    if(etati == 1.0)
    {
        etait = etati;
        cosThetaT = -cosThetaI;
        return 0.0;
    }

    const bool switch = cosThetaI > 0.0;
    etait = switch ? etati : rcp(etati);
    etati = switch ? rcp(etati) : etati;

    const float cosThetaTSqrd = negativeMultiplyAndAdd(negativeMultiplyAndAdd(cosThetaI, cosThetaI, 1.0), sqr(etati), 1.0);

    // total internal reflection
    if (cosThetaTSqrd <= 0.0)
    {
        cosThetaT = 0.0;
        return 1.0;
    }

    const float absCosThetaI = abs(cosThetaI);
    const float absCosThetaT = sqrt(cosThetaTSqrd);

    const float Rs = negativeMultiplyAndAdd(etait, absCosThetaT, absCosThetaI) / multiplyAndAdd(etait, absCosThetaT, absCosThetaI);
    const float Rp = negativeMultiplyAndAdd(etait, absCosThetaI, absCosThetaT) / multiplyAndAdd(etait, absCosThetaI, absCosThetaT);

    cosThetaT = switch ? -absCosThetaT : absCosThetaT;

    return 0.5 * (sqr(Rs) + sqr(Rp));
}

// Simliar to https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/#more-1921
float3 fresnelConductorExact(const float cosThetaI, const float3 eta, const float3 k)
{
    const float cos2ThetaI = sqr(cosThetaI);
    const float sin2ThetaI = 1.0 - cos2ThetaI;
    const float sin4ThetaI = sqr(sin2ThetaI);

    const float3 etaSqrd = sqr(eta);
    const float3 kSqrd = sqr(k);
    const float3 t0 = etaSqrd - kSqrd - sin2ThetaI;
    const float3 a2pb2 = safe_sqrt(t0 * t0 + 4.0 * kSqrd * etaSqrd);
    const float3 a = safe_sqrt(0.5 * (a2pb2 + t0));

    const float3 term1 = a2pb2 + cos2ThetaI;
    const float3 term2 = 2.0 * a * cosThetaI;

    const float3 Rs2 = (term1 - term2) / (term1 + term2);

    const float3 term3 = a2pb2 * cos2ThetaI + sin4ThetaI;
    const float3 term4 = term2 * sin2ThetaI;

    const float3 Rp2 = Rs2 * (term3 - term4) / (term3 + term4);

    return 0.5 * (Rp2 + Rs2);
}

float3 fresnelConductorApprox(const float cosThetaI, const float3 eta, const float3 k)
{
    const float cos2ThetaI = cosThetaI * cosThetaI;
    const float3 twoEtaCos2ThetaI = 2.0 * eta * cos2ThetaI;

    const float3 t0 = sqr(eta) + sqr(k);
    const float3 t1 = t0 * cos2ThetaI;
    const float3 Rs = (t0 - twoEtaCos2ThetaI + cos2ThetaI) / (t0 + twoEtaCos2ThetaI + cos2ThetaI);
    const float3 Rp = (t1 - twoEtaCos2ThetaI + 1.0) / (t1 + twoEtaCos2ThetaI + 1.0);

    return 0.5 * (Rp + Rs);
}

struct GGXDistribution
{
    __init(const float alphaU, const float alphaV)
    {
        this.alphaU = alphaU;
        this.alphaV = alphaV;
    }

    // VNDF
    float D(const float3 wm)
    {
        const float tan2Theta = tan2ThetaTS(wm);
        if (isinf(tan2Theta))
        {
            return 0.0;
        }

        float cos4Theta = sqr(cos2ThetaTS(wm));
        if (cos4Theta < EPSILON)
        {
            return 0.0;
        }

        const float scaledAlphaU = cosPhiTS(wm) / alphaU;
        const float scaledAlphaV = sinPhiTS(wm) / alphaV;
        const float sqrdAlpha = sqr(scaledAlphaU) + sqr(scaledAlphaV);
        const float e = tan2Theta * sqrdAlpha;

        return INV_PI / (alphaU * alphaV * cos4Theta * sqr(1.0 + e));
    }

    float D(const float3 w, const float3 wm)
    {
        return G1(w) / abs(cosThetaTS(w)) * abs(dot(w, wm)) * D(wm);
    }

    float lambda(const float3 w)
    {
        const float tan2Theta = tan2ThetaTS(w);
        if (isinf(tan2Theta))
        {
            return 0.0;
        }

        const float scaledAlphaU = cosPhiTS(w) * alphaU;
        const float scaledAlphaV = sinPhiTS(w) * alphaV;
        const float sqrdAlpha = sqr(scaledAlphaU) + sqr(scaledAlphaV);

        return 0.5 * (sqrt(1.0 + sqrdAlpha * tan2Theta) - 1.0);
    }

    // based on Sampling Visible GGX Normals with Spherical Caps https://arxiv.org/pdf/2306.05044.pdf
    Optional<MicrofacetSample> sample(const float3 wo)
    {
        // sample visible
        MicrofacetSample sample;

        const float2 rand = Random::rand2();

        float3 stretchedWo = normalize(float3(alphaU * wo.x, alphaV * wo.y, wo.z));
        float cosThetaO = cosThetaTS(stretchedWo);
        if (cosThetaO < 0.0)
        {
            stretchedWo = -stretchedWo;
            cosThetaO = -cosThetaO;
        }

        const float phi = TWO_PI * rand.x;
        const float z = 1.0 - (rand.y * (1.0 + cosThetaO));
        const float sinTheta = sqrt(clamp(1.0 - z * z, 0.0, 1.0));
        const float x = sinTheta * cos(phi);
        const float y = sinTheta * sin(phi);
        const float3 c = float3(x, y, z);

        const float3 h = c + stretchedWo;

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

    float alphaU;
    float alphaV;
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
    __init(const IBxDF bsdf)
    {
        this.bsdf0 = bsdf;
    }

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

    IBxDF bsdf0;
};


struct TwoSidedAdapter : IBxDFAdapter
{
    __init(const IBxDF bsdf)
    {
        this.bsdf0 = bsdf;
        this.bsdf1 = bsdf;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        bool flipped = false;
        float3 twoSidedWo = wo;
        if (cosThetaTS(twoSidedWo) < 0.0)
        {
            twoSidedWo.z *= -1;
            flipped = true;
        }

        const Optional<BxDFSample> result = flipped ? bsdf1.sample(twoSidedWo, mode) : bsdf0.sample(twoSidedWo, mode);

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
        return (EBxDFFlags)((uint)bsdf0.flags() | (uint)bsdf1.flags());
    }

    IBxDF bsdf0;
    IBxDF bsdf1;
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

        sample.wi = cosineHemisphereSample();

        if (cosThetaTS(wo) < 0.0)
        {
            sample.wi.z *= -1.0;
        }

        sample.pdf = cosineHemispherePdf(abs(cosThetaTS(sample.wi)));

        sample.f = diffuseReflectance * INV_PI;

        sample.flags = EBxDFFlags::diffuseReflection;

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

struct SmoothConductorBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;

        sample.wi = reflectTS(wo);

        sample.pdf = 1.0;

        const float absCosThetaI = abs(cosThetaTS(sample.wi));

        sample.f = fresnelConductorExact(absCosThetaI, eta, k) * specularReflectance / absCosThetaI;

        sample.flags = EBxDFFlags::specularReflection;

        return sample;

    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return float3(0.0);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return 0.0;
    }

    EBxDFFlags flags()
    {
        return EBxDFFlags::specularReflection;
    }

    float3 eta, k;
    float3 specularReflectance;
};

struct RoughConductorBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;

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

        sample.flags = EBxDFFlags::glossyReflection;

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
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

        if(dot(wm, wm) < EPSILON)
        {
            return float3(0.0);
        }

        wm = normalize(wm);

        const float D = distr.D(wm);

        const float3 F = fresnelConductorExact(abs(dot(wo, wm)), eta, k);

        const float G = distr.G(wo, wi);

        return D * G * F * specularReflectance / (4.0 * absCosThetaI * absCosThetaO);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
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

        if(dot(wm, wm) < EPSILON)
        {
            return 0.0;
        }

        wm = normalize(wm);
        // wm *= sign(cosThetaTS(wm));

        return distr.pdf(wo, wm) / (4.0 * abs(dot(wo, wm)));
    }

    EBxDFFlags flags()
    {
        return EBxDFFlags::glossyReflection;
    }

    float3 eta, k;
    float3 specularReflectance;

    GGXDistribution distr;

    // bool sampleVisible = true; // FIXME: Add option?
    // Distribution distribution = Distribution::GGX; // FIXME: Add option?
};

struct SmoothDielectricBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;

        float etati = eta;
        float etait = 0.0;
        float cosThetaT = 0.0;
        const float R = fresnelDielectric(cosThetaTS(wo), etati, etait, cosThetaT);

        const float T = 1.0 - R;

        if(Random::rand() < R) // reflection
        {
            sample.wi = reflectTS(wo);
            sample.pdf = R;

            sample.f = R * specularReflectance / abs(cosThetaTS(sample.wi));

            sample.flags = EBxDFFlags::specularReflection;
        }
        else // transmission
        {
            const Optional<float3> optionalDirection = refractTS(wo, cosThetaT, etati);

            if(optionalDirection == none)
            {
                return none;
            }

            sample.wi = optionalDirection.value;

            sample.pdf = T;

            sample.f = T * specularTransmittance / abs(cosThetaTS(sample.wi));

            if(mode == ETransportMode::radiance)
            {
                sample.f *= sqr(etati);
            }

            sample.flags = EBxDFFlags::specularTransmission;

            sample.eta = etait;
        }

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return float3(0.0);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return 0.0;
    }

    EBxDFFlags flags()
    {
        return (EBxDFFlags)(((eta == 1.0) ? (uint)EBxDFFlags::transmission : ((uint)EBxDFFlags::reflection | (uint)EBxDFFlags::transmission)) | (uint)EBxDFFlags::specular);
    }

    float eta;
    float3 specularReflectance;
    float3 specularTransmittance;
};

struct SmoothThinPlateDielectricBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;

        float etati = eta;
        float etait = 0.0;
        float cosThetaT = 0.0;
        float R = fresnelDielectric(abs(cosThetaTS(wo)), etati, etait, cosThetaT);

        if(R < 1.0)
        {
            // Account for internal reflections
            R += sqr(1.0 - R) * R / (1.0 - sqr(R));
        }

        const float T = 1.0 - R;

        if(Random::rand() < R) // reflection
        {
            sample.wi = reflectTS(wo);
            sample.pdf = R;

            sample.f = R * specularReflectance / abs(cosThetaTS(sample.wi));

            sample.flags = EBxDFFlags::specularReflection;
        }
        else // transmission
        {
            sample.wi = -wo;

            sample.pdf = T;

            sample.f = T * specularTransmittance / abs(cosThetaTS(sample.wi));

            sample.flags = EBxDFFlags::specularTransmission;

            sample.eta = 1.0;
        }

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return float3(0.0);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return 0.0;
    }

    EBxDFFlags flags()
    {
        return (EBxDFFlags)((uint)EBxDFFlags::reflection | (uint)EBxDFFlags::transmission | (uint)EBxDFFlags::specular);
    }

    float eta;
    float3 specularReflectance;
    float3 specularTransmittance;
};

struct RoughDielectricBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode)
    {
        BxDFSample sample;

        const Optional<MicrofacetSample> mfSample = distr.sample(wo);

        if (mfSample == none)
        {
            return none;
        }

        const float dotWoWm = dot(wo, mfSample.value.wm);
        float etati = eta;
        float etait = 0.0;
        float cosThetaT = 0.0;
        const float R = fresnelDielectric(dotWoWm, etati, etait, cosThetaT);

        const float T = 1.0 - R;

        if(Random::rand() < R) // reflection
        {
            sample.wi = reflect(wo, mfSample.value.wm);

            const float cosThetaI = cosThetaTS(sample.wi);
            const float cosThetaO = cosThetaTS(wo);

            if(!sameHemisphere(cosThetaI, cosThetaO))
            {
                return none;
            }

            sample.pdf = R * mfSample.value.pdf / (4.0 * abs(dotWoWm));

            const float D = distr.D(mfSample.value.wm);
            const float G = distr.G(wo, sample.wi);
            sample.f = D * G * R * specularReflectance / (4.0 * cosThetaI * cosThetaO);

            sample.flags = EBxDFFlags::glossyReflection;
        }
        else
        {
            Optional<float3> optionalDirection = refract(wo, mfSample.value.wm, cosThetaT, etati);

            if(optionalDirection == none)
            {
                return none;
            }

            sample.wi = optionalDirection.value;

            const float cosThetaI = cosThetaTS(sample.wi);
            const float cosThetaO = cosThetaTS(wo);

            if(sameHemisphere(cosThetaI, cosThetaO) || cosThetaI == 0.0)
            {
                return none;
            }

            const float dotWiWm = dot(sample.wi, mfSample.value.wm);
            const float dotWoWm = dot(wo, mfSample.value.wm);
            const float denom = sqr(dotWiWm + dotWoWm * etati);
            const float dwmDwi = abs(dotWiWm) / denom;

            sample.pdf = T * mfSample.value.pdf * dwmDwi;

            const float D = distr.D(mfSample.value.wm);
            const float G = distr.G(wo, sample.wi);
            sample.f = T * D * G * specularTransmittance * abs(dotWiWm * dotWoWm / (cosThetaI * cosThetaO * denom));

            if(mode == ETransportMode::radiance)
            {
                sample.f *= sqr(etati);
            }

            sample.flags = EBxDFFlags::glossyTransmission;
            sample.eta = etait;
        }

        return sample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI == 0.0 || cosThetaO == 0.0)
        {
            return float3(0.0);
        }

        const bool reflect = sameHemisphere(cosThetaI, cosThetaO);

        const float etait = reflect ? 1.0 : (cosThetaO > 0.0 ? eta : rcp(eta));

        float3 wm = etait * wi + wo;

        if(dot(wm, wm) < EPSILON)
        {
            return float3(0.0);
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        const float dotWiWm = dot(wi, wm);
        const float dotWoWm = dot(wo, wm);
        if (dotWoWm * cosThetaO < 0.0 || dotWiWm * cosThetaI < 0.0)
        {
            return float3(0.0);
        }

        const float D = distr.D(wm);

        const float etati = cosThetaO > 0.0 ? rcp(eta) : eta;
        const float F = fresnelDielectric(dotWoWm, etati, etait);

        const float G = distr.G(wo, wi);

        if(reflect)
        {
            return D * G * F * specularReflectance / abs(4.0 * cosThetaI * cosThetaO);
        }
        else
        {
            const float denom = sqr(dotWiWm + dotWoWm * etati) * cosThetaI * cosThetaO;
            float3 f = D * G * (1.0 - F) * specularTransmittance * abs(dotWiWm * dotWoWm / denom);

            if(mode == ETransportMode::radiance)
            {
                f *= sqr(etati);
            }

            return f;
        }
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const float cosThetaI = cosThetaTS(wi);
        const float cosThetaO = cosThetaTS(wo);

        if(cosThetaI == 0.0 || cosThetaO == 0.0)
        {
            return 0.0;
        }

        const bool reflect = sameHemisphere(cosThetaI, cosThetaO);

        const float etait = reflect ? 1.0 : (cosThetaO > 0.0 ? eta : rcp(eta));

        float3 wm = etait * wi + wo;

        if(dot(wm, wm) < EPSILON)
        {
            return 0.0;
        }

        wm = normalize(wm);
        wm *= sign(cosThetaTS(wm));

        const float dotWiWm = dot(wi, wm);
        const float dotWoWm = dot(wo, wm);
        if (dotWoWm * cosThetaO < 0.0 || dotWiWm * cosThetaI < 0.0)
        {
            return 0.0;
        }

        const float etati = cosThetaO > 0.0 ? rcp(eta) : eta;
        const float R = fresnelDielectric(dotWoWm, etati, etait);
        const float T = 1.0 - R;

        if(reflect)
        {
            return R * distr.pdf(wo, wm) / (4.0 * abs(dotWoWm));
        }
        else
        {
            const float denom = sqr(dotWiWm + dotWoWm * etati);
            const float dwmDwi = abs(dotWiWm) / denom;

            return T * distr.pdf(wo, wm) * dwmDwi;
        }
    }

    EBxDFFlags flags()
    {
        return (EBxDFFlags)(((eta == 1.0) ? (uint)EBxDFFlags::transmission : ((uint)EBxDFFlags::reflection | (uint)EBxDFFlags::transmission)) | (uint)EBxDFFlags::glossy);
    }

    float eta;
    float3 specularReflectance;
    float3 specularTransmittance;

    GGXDistribution distr;

    // bool sampleVisible = true; // FIXME: Add option?
    // Distribution distribution = Distribution::GGX; // FIXME: Add option?
};

struct LayeredBSDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode);
    float3 f(const float3 wo, const float3 wi, const ETransportMode mode);
    float pdf(const float3 wo, const float3 wi, const ETransportMode mode);

    EBxDFFlags flags();

    IBxDF topLayer;
    IBxDF bottomLayer;

    float thickness = 1.0;
    float sigmaT = 1.0;
    float henyeyGreensteinG = 0.0;
    uint maxDepth = 64;
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

        bsdf.eta = this.eta;

        return bsdf;
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

    typedef SmoothThinPlateDielectricBSDF RealBxDF;

    SmoothThinPlateDielectricBSDF createBxDF(const float2 uv)
    {
        SmoothThinPlateDielectricBSDF bsdf;
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

        bsdf.eta = this.eta;

        return bsdf;
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

        return bsdf;
    }

    float3 specularReflectance;
    int specularReflectanceTexture;
    float3 specularTransmittance;
    int specularTransmittanceTexture;
    float2 alpha;
    int alphaTexture;
    float eta;
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
                    return SmoothConductorMaterial(this);
                }
            case EBxDFType::roughConductor:
                {
                    if(max(this.alpha.x, this.alpha.y) < EPSILON)
                    {
                        return SmoothConductorMaterial(this);
                    }

                    return RoughConductorMaterial(this);
                }
            case EBxDFType::smoothDielectricThin:
                {
                    return SmoothThinPlateDielectricMaterial(this);
                }
            case EBxDFType::smoothDielectric:
                {
                    return SmoothDielectricMaterial(this);
                }
            case EBxDFType::roughDielectric:
                {
                    if(this.interiorIor / this.exteriorIor == 1.0 || max(this.alpha.x, this.alpha.y) < EPSILON)
                    {
                        return SmoothDielectricMaterial(this);
                    }

                    return RoughDielectricMaterial(this);
                }
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