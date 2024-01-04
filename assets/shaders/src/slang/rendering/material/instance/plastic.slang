__exported import rendering.material.materialInterface;
import rendering.material.ggx;
import rendering.material.instance.diffuse;
import rendering.material.instance.dielectric;
import rendering.material.instance.layered;
import rendering.renderingUtils;

struct SmoothPlasticMaterialInstance : IMaterialInstance
{
    __init(const float3 diffuseReflectance, const float3 specularReflectance, const float eta)
    {
        this.diffuseReflectance = diffuseReflectance;
        this.specularReflectance = specularReflectance;
        this.eta = eta;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        const SmoothDielectricBSDF topLayer = SmoothDielectricBSDF(specularReflectance, specularReflectance, eta);
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        const Optional<BxDFSample> optionalSample = LayeredBSDF(topLayer, bottomLayer).sample(wo, mode, sampleFlags);

        if(optionalSample == none)
        {
            return none;
        }
        if(!sameHemisphere(wo, optionalSample.value.wi))
        {
            return none;
        }

        return optionalSample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(!sameHemisphere(wo, wi))
        {
            return float3(0.0);
        }

        const SmoothDielectricBSDF topLayer = SmoothDielectricBSDF(specularReflectance, specularReflectance, eta);
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        return LayeredBSDF(topLayer, bottomLayer).f(wo, wi, mode);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        if(!sameHemisphere(wo, wi))
        {
            return 0.0;
        }

        const SmoothDielectricBSDF topLayer = SmoothDielectricBSDF(specularReflectance, specularReflectance, eta);
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        return LayeredBSDF(topLayer, bottomLayer).pdf(wo, wi, mode, sampleFlags);
    }

    EBxDFFlags flags()
    {
        const SmoothDielectricBSDF topLayer = SmoothDielectricBSDF(specularReflectance, specularReflectance, eta);
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        return LayeredBSDF(topLayer, bottomLayer).flags();
    }

    float eta;
    float3 diffuseReflectance;
    float3 specularReflectance;
};

struct RoughPlasticMaterialInstance : IMaterialInstance
{
    __init(const float3 diffuseReflectance, const float3 specularReflectance, const float eta, const float alpha)
    {
        this.diffuseReflectance = diffuseReflectance;
        this.specularReflectance = specularReflectance;
        this.eta = eta;
        this.alpha = alpha;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        const RoughDielectricBSDF topLayer = RoughDielectricBSDF(specularReflectance, specularReflectance, eta, GGXDistribution(alpha, alpha));
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        const Optional<BxDFSample> optionalSample = LayeredBSDF(topLayer, bottomLayer).sample(wo, mode, sampleFlags);

        if(optionalSample == none)
        {
            return none;
        }
        if(!sameHemisphere(wo, optionalSample.value.wi))
        {
            return none;
        }

        return optionalSample;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(!sameHemisphere(wo, wi))
        {
            return float3(0.0);
        }

        const RoughDielectricBSDF topLayer = RoughDielectricBSDF(specularReflectance, specularReflectance, eta, GGXDistribution(alpha, alpha));
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        return LayeredBSDF(topLayer, bottomLayer).f(wo, wi, mode);
    }

    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        if(!sameHemisphere(wo, wi))
        {
            return 0.0;
        }

        const RoughDielectricBSDF topLayer = RoughDielectricBSDF(specularReflectance, specularReflectance, eta, GGXDistribution(alpha, alpha));
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        return LayeredBSDF(topLayer, bottomLayer).pdf(wo, wi, mode, sampleFlags);
    }

    EBxDFFlags flags()
    {
        const RoughDielectricBSDF topLayer = RoughDielectricBSDF(specularReflectance, specularReflectance, eta, GGXDistribution(alpha, alpha));
        const DiffuseBSDF bottomLayer = DiffuseBSDF(diffuseReflectance);
        return LayeredBSDF(topLayer, bottomLayer).flags();
    }

    float eta;
    float alpha;
    float3 diffuseReflectance;
    float3 specularReflectance;
};
