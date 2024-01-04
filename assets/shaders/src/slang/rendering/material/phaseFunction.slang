import rendering.renderingUtils;
import core.random;

struct PhaseFunctionSample
{
    float3 wi;
    float pdf;
    float p;
};

interface IPhaseFunction
{
    Optional<PhaseFunctionSample> sample(const float3 wo);
    float3 p(const float3 wo, const float3 wi);
    float pdf(const float3 wo, const float3 wi);
};


struct HenyeyGreensteinPhaseFunction : IPhaseFunction
{
    __init(const float g)
    {
        this.g = clamp(g, -0.99, 0.99); // clamping to avoid singularities
    }

    Optional<PhaseFunctionSample> sample(const float3 wo)
    {
        PhaseFunctionSample pfSample;
        const float2 rand = Random::rand2();

        const float gSqrd = sqr(g);

        float cosTheta;
        if(abs(g) < EPSILON)
        {
            cosTheta = 1.0 - 2.0 * rand.x;
        }
        else
        {
            cosTheta = -1.0 / (2.0 * g) * (1.0 + gSqrd - sqr((1.0 - gSqrd) / (1.0 + g - 2.0 * g * rand.x)));
        }

        const float sinTheta = safe_sqrt(1.0 - sqr(cosTheta));
        const float phi = TWO_PI * rand.y;

        const float3 w = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
        const CoordinateSpace space = CoordinateSpace(wo);

        pfSample.wi = normalize(space.tangentToWorld(w));
        pfSample.pdf = hG(cosTheta);
        pfSample.p = 1.0;

        return pfSample;
    }

    float3 p(const float3 wo, const float3 wi)
    {
        return float3(hG(dot(wo, wi)));
    }

    float pdf(const float3 wo, const float3 wi)
    {
        return hG(dot(wo, wi));
    }

    float hG(const float cosTheta)
    {
        const float gSqrd = sqr(g);
        const float denom = 1.0 + gSqrd + 2.0 * g * cosTheta;
        return INV_FOUR_PI * (1.0 - gSqrd) / (denom * safe_sqrt(denom));
    }

    float g;
};
