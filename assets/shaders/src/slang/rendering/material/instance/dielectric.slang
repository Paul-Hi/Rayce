__exported import rendering.material.materialInterface;
import rendering.material.ggx;
import rendering.renderingUtils;
import core.random;

struct SmoothDielectricBSDF : ILayerableBxDF
{
    __init(const float3 specularReflectance, const float3 specularTransmittance, const float eta)
    {
        this.specularReflectance = specularReflectance;
        this.specularTransmittance = specularTransmittance;
        this.eta = eta;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        BxDFSample sample;

        float etati = eta;
        float etait = 0.0;
        float cosThetaT = 0.0;
        const float R = fresnelDielectric(cosThetaTS(wo), etati, etait, cosThetaT);
        const float T = 1.0 - R;

        float pr = R, pt = T;
        if (!sampleReflection(sampleFlags)) pr = 0.0;
        if (!sampleTransmission(sampleFlags)) pt = 0.0;
        if (pr == 0.0 && pt == 0.0)
            return none;

        if(Random::rand() < pr / (pr + pt)) // reflection
        {
            sample.wi = reflectTS(wo);
            sample.pdf = pr / (pr + pt);

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

            sample.pdf = pt / (pr + pt);

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

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
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

struct SmoothThinPlateDielectricBSDF : ILayerableBxDF
{
    __init(const float3 specularReflectance, const float3 specularTransmittance, const float eta)
    {
        this.specularReflectance = specularReflectance;
        this.specularTransmittance = specularTransmittance;
        this.eta = eta;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
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

        float pr = R, pt = T;
        if (!sampleReflection(sampleFlags)) pr = 0.0;
        if (!sampleTransmission(sampleFlags)) pt = 0.0;
        if (pr == 0.0 && pt == 0.0)
            return none;

        if(Random::rand() < pr / (pr + pt)) // reflection
        {
            sample.wi = reflectTS(wo);
            sample.pdf = pr / (pr + pt);

            sample.f = R * specularReflectance / abs(cosThetaTS(sample.wi));

            sample.flags = EBxDFFlags::specularReflection;
        }
        else // transmission
        {
            sample.wi = -wo;

            sample.pdf = pt / (pr + pt);

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

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
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

struct RoughDielectricBSDF : ILayerableBxDF
{
    __init(const float3 specularReflectance, const float3 specularTransmittance, const float eta, const GGXDistribution distr)
    {
        this.specularReflectance = specularReflectance;
        this.specularTransmittance = specularTransmittance;
        this.eta = eta;
        this.distr = distr;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
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

        float pr = R, pt = T;
        if (!sampleReflection(sampleFlags)) pr = 0.0;
        if (!sampleTransmission(sampleFlags)) pt = 0.0;
        if (pr == 0.0 && pt == 0.0)
            return none;

        if(Random::rand() < pr / (pr + pt)) // reflection
        {
            sample.wi = reflect(wo, mfSample.value.wm);

            const float cosThetaI = cosThetaTS(sample.wi);
            const float cosThetaO = cosThetaTS(wo);

            if(!sameHemisphere(cosThetaI, cosThetaO))
            {
                return none;
            }

            sample.pdf = pr / (pr + pt) * mfSample.value.pdf / (4.0 * abs(dotWoWm));

            const float D = distr.D(mfSample.value.wm);
            const float G = distr.G(wo, sample.wi);
            sample.f = D * G * R * specularReflectance / (4.0 * cosThetaI * cosThetaO);

            sample.flags = EBxDFFlags::glossyReflection;
        }
        else // transmission
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

            sample.pdf = pt / (pr + pt) * mfSample.value.pdf * dwmDwi;

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

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
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

        float pr = R, pt = T;
        if (!sampleReflection(sampleFlags)) pr = 0.0;
        if (!sampleTransmission(sampleFlags)) pt = 0.0;
        if (pr == 0.0 && pt == 0.0)
            return 0.0;

        if(reflect)
        {
            return pr / (pr + pt) * distr.pdf(wo, wm) / (4.0 * abs(dotWoWm));
        }
        else
        {
            const float denom = sqr(dotWiWm + dotWoWm * etati);
            const float dwmDwi = abs(dotWiWm) / denom;

            return pt / (pr + pt) * distr.pdf(wo, wm) * dwmDwi;
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

struct SmoothDielectricMaterialInstance : IMaterialInstance
{
    __init(const SmoothDielectricBSDF bsdf)
    {
        this.bsdf = bsdf;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        return bsdf.sample(wo, mode, sampleFlags);
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return bsdf.f(wo, wi, mode);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        return bsdf.pdf(wo, wi, mode, sampleFlags);
    }

    EBxDFFlags flags()
    {
        return bsdf.flags();
    }

    SmoothDielectricBSDF bsdf;
};

struct SmoothThinPlateDielectricMaterialInstance : IMaterialInstance
{
    __init(const SmoothThinPlateDielectricBSDF bsdf)
    {
        this.bsdf = bsdf;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        return bsdf.sample(wo, mode, sampleFlags);
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return bsdf.f(wo, wi, mode);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        return bsdf.pdf(wo, wi, mode, sampleFlags);
    }

    EBxDFFlags flags()
    {
        return bsdf.flags();
    }

    SmoothThinPlateDielectricBSDF bsdf;
};

struct RoughDielectricMaterialInstance : IMaterialInstance
{
    __init(const RoughDielectricBSDF bsdf)
    {
        this.bsdf = bsdf;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        return bsdf.sample(wo, mode, sampleFlags);
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        return bsdf.f(wo, wi, mode);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        return bsdf.pdf(wo, wi, mode, sampleFlags);
    }

    EBxDFFlags flags()
    {
        return bsdf.flags();
    }

    RoughDielectricBSDF bsdf;
};
