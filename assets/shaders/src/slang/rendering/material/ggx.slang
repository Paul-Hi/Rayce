import rendering.renderingUtils;
import core.random;

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
