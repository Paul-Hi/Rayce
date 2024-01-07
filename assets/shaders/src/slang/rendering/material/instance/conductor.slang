__exported import rendering.material.materialInterface;
import rendering.material.ggx;
import rendering.renderingUtils;

struct SmoothConductorBSDF : ILayerableBxDF
{
    __init(const float3 specularReflectance, const float3 eta, const float3 k)
    {
        this.specularReflectance = specularReflectance;
        this.eta = eta;
        this.k = k;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        if(!sampleReflection(sampleFlags))
        {
            return none;
        }

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

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
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

struct RoughConductorBSDF : ILayerableBxDF
{
    __init(const float3 specularReflectance, const float3 eta, const float3 k, const GGXDistribution distr)
    {
        this.specularReflectance = specularReflectance;
        this.eta = eta;
        this.k = k;
        this.distr = distr;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        if(!sampleReflection(sampleFlags))
        {
            return none;
        }

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

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        if(!sampleReflection(sampleFlags))
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

struct SmoothConductorMaterialInstance : IMaterialInstance
{
    __init(const SmoothConductorBSDF bsdf)
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

    SmoothConductorBSDF bsdf;
};

struct RoughConductorMaterialInstance : IMaterialInstance
{
    __init(const RoughConductorBSDF bsdf)
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

    RoughConductorBSDF bsdf;
};
