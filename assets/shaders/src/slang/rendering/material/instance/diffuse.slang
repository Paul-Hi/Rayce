__exported import rendering.material.materialInterface;
import rendering.renderingUtils;

struct DiffuseBSDF : ILayerableBxDF
{
    __init(const float3 diffuseReflectance)
    {
        this.diffuseReflectance = diffuseReflectance;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        if(!sampleReflection(sampleFlags))
        {
            return none;
        }

        BxDFSample sample;

        sample.wi = cosineHemisphereSample();

        if (cosThetaTS(wo) < 0.0)
        {
            return none;
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

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        if(!sameHemisphere(wo, wi) || !sampleReflection(sampleFlags))
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

struct DiffuseMaterialInstance : IMaterialInstance
{
    __init(const DiffuseBSDF bsdf)
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

    DiffuseBSDF bsdf;
};
