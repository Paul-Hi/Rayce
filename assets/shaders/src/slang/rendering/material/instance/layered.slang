__exported import rendering.material.materialInterface;
import rendering.material.ggx;
import rendering.material.phaseFunction;
import rendering.renderingUtils;
import core.random;

// https://shuangz.com/projects/layered-sa18/layered-sa18.pdf
// rough mix between pbrt and falcor implementations
struct LayeredBSDF : IBxDF
{
    __init(const ILayerableBxDF topLayer, const ILayerableBxDF bottomLayer, const float thickness = 0.01, const float3 albedo = float3(0.0), const float sigmaT = 1.0, const float henyeyGreensteinG = 0.0, const uint maxDepth = 10, const uint sampleCount = 1)
    {
        this.topLayer = topLayer;
        this.bottomLayer = bottomLayer;
        this.albedo = albedo;
        this.thickness = thickness;
        this.sigmaT = sigmaT;
        this.henyeyGreensteinG = henyeyGreensteinG;
        this.maxDepth = maxDepth;
        this.sampleCount = sampleCount;
    }

    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        // NOTE: Ignoring sampleFlags

        const bool enterTop = cosThetaTS(wo) > 0.0;

        BxDFSample sample;

        const HenyeyGreensteinPhaseFunction phase = HenyeyGreensteinPhaseFunction(henyeyGreensteinG);

        // sample a wo
        const Optional<BxDFSample> woSample = sampleTopOrBottom(enterTop, wo, mode);
        if (woSample == none)
        {
            none;
        }
        sample = woSample.value;
        if (sample.isReflection())
        {
            sample.proportionalPDF = true; // since we use the approximation in pdf()

            return sample;
        }

        float3 w = sample.wi;
        bool specularPath = sample.isSpecular();
        float3 f = sample.f * abs(cosThetaTS(w));
        float pdf = sample.pdf;
        float z = enterTop ? thickness : 0.0;

        for(int depth = 0; depth < maxDepth; ++depth)
        {
            // russian roulette
            const float maxRRBeta = max(max(f.x, f.y), f.z) / pdf;
            if (maxRRBeta < 0.25 && depth > 3)
            {
                const float q = max(1.0 - maxRRBeta, 0.0);
                if (Random::rand() < q)
                {
                    return none;
                }

                pdf *= (1.0 - q);
            }
            const float cosTheta = cosThetaTS(w);
            if(cosTheta == 0.0)
            {
                return none;
            }

            if (any(albedo > 0.0))
            {
                const float dz = -log(1.0 - Random::rand()) * abs(cosTheta) / sigmaT; // exponential sample
                const float zp = cosTheta > 0 ? (z + dz) : (z - dz);
                if (z == zp)
                {
                    continue;
                }
                if (zp > 0.0 && zp < thickness)
                {
                    const Optional<PhaseFunctionSample> pfSample = phase.sample(-w);
                    if(pfSample == none)
                    {
                        continue;
                    }
                    f *= albedo * pfSample.value.p;
                    pdf *= pfSample.value.pdf;
                    specularPath = false;
                    w = pfSample.value.wi;
                    z = zp;

                    continue;
                }

                z = clamp(zp, 0.0, thickness);
            }
            else
            {
                // no media between layers
                z = (z == thickness) ? 0.0 : thickness;
                f *= transmittance(thickness, w);
            }

            const bool interface = (z == thickness);

            const Optional<BxDFSample> bxdfSample = sampleTopOrBottom(interface, -w, mode);
            if (bxdfSample == none)
            {
                none;
            }

            f *= bxdfSample.value.f;
            pdf *= bxdfSample.value.pdf;
            w = bxdfSample.value.wi;
            specularPath &= bxdfSample.value.isSpecular();

            if (bxdfSample.value.isTransmission())
            {
                sample.wi = w;
                sample.pdf = pdf;
                sample.f = f;
                sample.flags = sameHemisphere(wo, w) ? EBxDFFlags::reflection : EBxDFFlags::transmission;
                sample.flags = (EBxDFFlags)((uint)sample.flags | (specularPath ? (uint)(EBxDFFlags::specular) : (uint)(EBxDFFlags::glossy)));
                sample.proportionalPDF = true; // since we approximate in pdf()
                sample.eta = 1.0;
                return sample;
            }

            f *= abs(cosThetaTS(bxdfSample.value.wi));
        }

        return none;
    }

    float3 f(const float3 wo, const float3 wi, const ETransportMode mode)
    {
        const bool enterTop = cosThetaTS(wo) > 0.0;
        const bool exitTop = cosThetaTS(wi) > 0.0;

        const float exitZ = exitTop ? thickness : 0.0;
        const bool exitNonSpecular = !isSpecular(flagsTopOrBottom(exitTop));
        const bool nonExitNonSpecular = !isSpecular(flagsTopOrBottom(!exitTop));

        float3 f = float3(0.0);

        if(enterTop == exitTop) // reflection at entrance interface?
        {
            f += sampleCount * fTopOrBottom(enterTop, wo, wi, mode);
        }

        const HenyeyGreensteinPhaseFunction phase = HenyeyGreensteinPhaseFunction(henyeyGreensteinG);

        for(int s = 0; s < sampleCount; ++s)
        {
            // sample a wo
            const Optional<BxDFSample> woSample = sampleTopOrBottom(enterTop, wo, mode, EBxDFSampleFlags::transmission);
            if (woSample == none)
            {
                continue;
            }

            // sample a wi
            const Optional<BxDFSample> wiSample = sampleTopOrBottom(exitTop, wi, mode, EBxDFSampleFlags::transmission);
            if (wiSample == none)
            {
                continue;
            }

            float3 w = woSample.value.wi;
            float3 throughput = woSample.value.f * abs(cosThetaTS(w)) / woSample.value.pdf;
            float z = enterTop ? thickness : 0.0;

            for(int depth = 0; depth < maxDepth; ++depth)
            {
                // russian roulette
                const float maxRRBeta = max(max(throughput.x, throughput.y), throughput.z);
                if (maxRRBeta < 0.25 && depth > 3)
                {
                    const float q = max(1.0 - maxRRBeta, 0.0);
                    if (Random::rand() < q)
                    {
                        break;
                    }

                    throughput *= 1.0 / (1.0 - q);
                }

                if (any(albedo > 0.0))
                {
                    const float cosTheta = cosThetaTS(w);
                    const float dz = -log(1.0 - Random::rand()) * abs(cosTheta) / sigmaT; // exponential sample
                    const float zp = cosTheta > 0 ? (z + dz) : (z - dz);
                    if (z == zp)
                    {
                        continue;
                    }
                    if (zp > 0.0 && zp < thickness)
                    {
                        float wt = 1.0;
                        if (exitNonSpecular)
                        {
                            wt = powerHeuristic(wiSample.value.pdf, phase.pdf(-w, -wiSample.value.wi));
                        }

                        f += throughput * albedo * phase.p(-w, -wiSample.value.wi) * wt *
                                transmittance(zp - exitZ, wiSample.value.wi) * wiSample.value.f / wiSample.value.pdf;

                        const Optional<PhaseFunctionSample> pfSample = phase.sample(-w);
                        if(pfSample == none)
                        {
                            continue;
                        }
                        throughput *= albedo * pfSample.value.p / pfSample.value.pdf;
                        w = pfSample.value.wi;
                        z = zp;

                        if (((z < exitZ && cosTheta > 0) || (z > exitZ && cosTheta < 0)) && exitNonSpecular)
                        {
                            const float3 fExit = fTopOrBottom(exitTop, -w, wi, mode);
                            if (any(fExit > 0.0))
                            {
                                const float exitPDF = pdfTopOrBottom(exitTop, -w, wi, mode, EBxDFSampleFlags::transmission);
                                const float wt = powerHeuristic(pfSample.value.pdf, exitPDF);
                                f += throughput * transmittance(zp - exitZ, pfSample.value.wi) * fExit * wt;
                            }
                        }

                        continue;
                    }

                    z = clamp(zp, 0.0, thickness);
                }
                else
                {
                    // no media between layers
                    z = (z == thickness) ? 0.0 : thickness;
                    throughput *= transmittance(thickness, w);
                }

                const Optional<BxDFSample> bxdfSample = sampleTopOrBottom(z == exitZ ? exitTop : !exitTop, -w, mode, EBxDFSampleFlags::reflection);

                if (z == exitZ)
                {
                    if (bxdfSample == none)
                    {
                        break;
                    }

                    throughput *= bxdfSample.value.f * abs(cosThetaTS(bxdfSample.value.wi)) / bxdfSample.value.pdf;
                    w = bxdfSample.value.wi;
                }
                else
                {
                    if (nonExitNonSpecular)
                    {
                        float wt = 1.0;
                        if (exitNonSpecular)
                        {
                            wt = powerHeuristic(wiSample.value.pdf, pdfTopOrBottom(!exitTop, -w, -wiSample.value.wi, mode));
                        }

                        f += throughput * fTopOrBottom(!exitTop, -w, -wiSample.value.wi, mode) * abs(cosThetaTS(wiSample.value.wi)) * wt *
                                transmittance(thickness, wiSample.value.wi) * wiSample.value.f / wiSample.value.pdf;
                    }

                    if (bxdfSample == none)
                    {
                        break;
                    }
                    if (bxdfSample.value.isTransmission())
                    {
                        break;
                    }

                    throughput *= bxdfSample.value.f * abs(cosThetaTS(bxdfSample.value.wi)) / bxdfSample.value.pdf;
                    w = bxdfSample.value.wi;

                    if (exitNonSpecular)
                    {
                        const float3 fExit = fTopOrBottom(exitTop, -w, wi, mode);
                        if (any(fExit > 0.0))
                        {
                            float wt = 1.0;
                            if (nonExitNonSpecular)
                            {
                                const float exitPDF = pdfTopOrBottom(exitTop, -w, wi, mode, EBxDFSampleFlags::transmission);
                                float wt = powerHeuristic(bxdfSample.value.pdf, exitPDF);
                            }
                            f += throughput * transmittance(thickness, bxdfSample.value.wi) * fExit * wt;
                        }
                    }
                }
            }
        }

        return f / sampleCount;
    }

    // TRT approximation
    float pdf(const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags)
    {
        // NOTE: Ignoring sampleFlags

        const bool enterTop = cosThetaTS(wo) > 0.0;
        const bool exitTop = cosThetaTS(wi) > 0.0;

        float pdf = 0.0;
        if(enterTop == exitTop) // reflection at entrance interface?
        {
            pdf += sampleCount * pdfTopOrBottom(enterTop, wo, wi, mode, EBxDFSampleFlags::reflection);
        }

        const HenyeyGreensteinPhaseFunction phase = HenyeyGreensteinPhaseFunction(henyeyGreensteinG);

        for(int s = 0; s < sampleCount; ++s)
        {
            // TRT
            if(enterTop == exitTop)
            {
                const bool transmissionInterface = enterTop;
                const bool reflectionInterface = !transmissionInterface;

                // sample a wo
                const Optional<BxDFSample> woSample = sampleTopOrBottom(transmissionInterface, wo, mode, EBxDFSampleFlags::transmission);
                if (woSample != none)
                {
                    // sample a wi
                    const Optional<BxDFSample> wiSample = sampleTopOrBottom(transmissionInterface, wi, (ETransportMode)!(uint)mode, EBxDFSampleFlags::transmission);
                    if (wiSample == none)
                    {
                            // do stuff
                        if (!isNonSpecular(flagsTopOrBottom(transmissionInterface)))
                        {
                            pdf += pdfTopOrBottom(reflectionInterface, -woSample.value.wi, -wiSample.value.wi, mode);
                        }
                        else
                        {
                            Optional<BxDFSample> bxdfSample = sampleTopOrBottom(reflectionInterface, -woSample.value.wi, mode);
                            if(bxdfSample != none)
                            {
                                if (!isNonSpecular(flagsTopOrBottom(reflectionInterface)))
                                {
                                    pdf += pdfTopOrBottom(transmissionInterface, -bxdfSample.value.wi, wi, mode);
                                }
                                else
                                {
                                    const float rPDF = pdfTopOrBottom(reflectionInterface, -woSample.value.wi, -wiSample.value.wi, mode);
                                    float wt = powerHeuristic(wiSample.value.pdf, rPDF);
                                    pdf += wt * rPDF;

                                    const float tPDF = pdfTopOrBottom(transmissionInterface, -bxdfSample.value.wi, wi, mode);
                                    wt = powerHeuristic(bxdfSample.value.pdf, tPDF);
                                    pdf += wt * tPDF;
                                }
                            }
                        }
                    }
                }
            }
            else // TT
            {
                const bool transmissionInterfaceO = enterTop;
                const bool transmissionInterfaceI = !transmissionInterfaceO;

                // sample a wo
                const Optional<BxDFSample> woSample = sampleTopOrBottom(transmissionInterfaceO, wo, mode);
                if (woSample == none)
                {
                    continue;
                }
                if (woSample.value.isReflection())
                {
                    continue;
                }

                // sample a wi
                const Optional<BxDFSample> wiSample = sampleTopOrBottom(transmissionInterfaceI, wi, (ETransportMode)!(uint)mode);
                if (wiSample == none)
                {
                    continue;
                }
                if (wiSample.value.isReflection())
                {
                    continue;
                }

                if (isSpecular(flagsTopOrBottom(transmissionInterfaceO)))
                {
                    pdf += pdfTopOrBottom(transmissionInterfaceI, -woSample.value.wi, wi, mode);
                }
                else if (isSpecular(flagsTopOrBottom(transmissionInterfaceI)))
                {
                    pdf += pdfTopOrBottom(transmissionInterfaceO, wo, -wiSample.value.wi, mode);
                }
                else
                {
                    pdf += 0.5 * (pdfTopOrBottom(transmissionInterfaceI, -woSample.value.wi, wi, mode)
                                + pdfTopOrBottom(transmissionInterfaceO, wo, -wiSample.value.wi, mode));
                }
            }
        }

        return lerp(INV_FOUR_PI, pdf / sampleCount, 0.9);
    }

    EBxDFFlags flags()
    {
        const EBxDFFlags topFlags = topLayer.flags();
        const EBxDFFlags bottomFlags = bottomLayer.flags();

        EBxDFFlags flags = EBxDFFlags::reflection;
        if (isSpecular(topFlags))
        {
            flags = (EBxDFFlags)((uint)flags | (uint)EBxDFFlags::specular);
        }

        if (isDiffuse(topFlags) || isDiffuse(bottomFlags) || any(albedo > 0.0))
        {
            flags = (EBxDFFlags)((uint)flags | (uint)EBxDFFlags::diffuse);
        }
        else if (isGlossy(topFlags) || isGlossy(bottomFlags))
        {
            flags = (EBxDFFlags)((uint)flags | (uint)EBxDFFlags::glossy);
        }

        if (isTransmissive(topFlags) && isTransmissive(bottomFlags))
        {
            flags = (EBxDFFlags)((uint)flags | (uint)EBxDFFlags::transmission);
        }

        return flags;
    }

    Optional<BxDFSample> sampleTopOrBottom(const bool top, const float3 wo, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        if(top)
        {
            return topLayer.sample(wo, mode, sampleFlags);
        }
        else
        {
            return bottomLayer.sample(wo, mode, sampleFlags);
        }
    }

    float3 fTopOrBottom(const bool top, const float3 wo, const float3 wi, const ETransportMode mode)
    {
        if(top)
        {
            return topLayer.f(wo, wi, mode);
        }
        else
        {
            return bottomLayer.f(wo, wi, mode);
        }
    }

    float pdfTopOrBottom(const bool top, const float3 wo, const float3 wi, const ETransportMode mode, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all)
    {
        if(top)
        {
            return topLayer.pdf(wo, wi, mode, sampleFlags);
        }
        else
        {
            return bottomLayer.pdf(wo, wi, mode, sampleFlags);
        }
    }

    EBxDFFlags flagsTopOrBottom(const bool top)
    {
        if(top)
        {
            return topLayer.flags();
        }
        else
        {
            return bottomLayer.flags();
        }
    }

    float transmittance(const float z, const float3 w)
    {
        return exp(-sigmaT * abs(z / cosThetaTS(w)));
    }

    ILayerableBxDF topLayer;
    ILayerableBxDF bottomLayer;
    float3 albedo;
    float thickness;
    float sigmaT;
    float henyeyGreensteinG;
    uint maxDepth;
    uint sampleCount;
};
