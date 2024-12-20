import rendering.renderpass.integrator;
import core.data;
import core.random;

[shader("raygeneration")]
void main()
{
    int2 pixelIndex = int2(DispatchRaysIndex().xy);
    float4 accumColor = gPushConstants.frame > 0 ? gAccumulationImage[pixelIndex] : float4(0.0, 0.0, 0.0, 0.0);
    accumColor.rgb *= accumColor.w;
    const float2 pixelCorner = float2(DispatchRaysIndex().xy);

    float3 color = float3(0.0);
    Random::init(uint2(interleave32Bit(DispatchRaysIndex().xy), gPushConstants.frame + 290797));

    // Anti Aliasing
    float2 uv = (pixelCorner + shirley(Random::rand2())) / float2(DispatchRaysDimensions().xy - uint2(1));
    uv = uv * 2.0 - 1.0;

    // FIXME: Precompiling the different integrator shaders should increase performance...
    let integratorT = gPushConstants.integrator;

    switch(integratorT)
    {
        case EIntegratorType::direct:
            {
                DirectIntegrator integrator;
                color += integrator.estimate(uv).L;
                break;
            }
        case EIntegratorType::path:
            {
                PathIntegrator integrator;
                color += integrator.estimate(uv).L;
                break;
            }
        case EIntegratorType::debugDepth:
            {
                DebugIntegrator integrator;
                color += integrator.estimate(uv).depth;
                break;
            }
        case EIntegratorType::debugNormals:
            {
                DebugIntegrator integrator;
                color += integrator.estimate(uv).normal * 0.5 + 0.5;
                break;
            }
        case EIntegratorType::debugReflectance:
            {
                DebugIntegrator integrator;
                color += integrator.estimate(uv).reflectance;
                break;
            }
        case EIntegratorType::debugEmission:
            {
                DebugIntegrator integrator;
                color += integrator.estimate(uv).emission;
                break;
            }
        default:
            break;
    }

    accumColor.rgb += color;
    accumColor.w += 1.0;
    accumColor.rgb /= accumColor.w;

    gAccumulationImage[pixelIndex] = accumColor;
    const float4 resultColor = float4(linearTosRGB(toneMapACES(accumColor.rgb)), 1.0);
    gResultImage[pixelIndex] = resultColor;
}
