import core.globals;

[anyValueSize(36)]
interface IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);
    float3 f(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);
    float pdf(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);

    EBxDFFlags flags();
};

[anyValueSize(36)]
interface ILayerableBxDF : IBxDF
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);
    float3 f(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);
    float pdf(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);

    EBxDFFlags flags();
};

[anyValueSize(60)]
interface IMaterialInstance
{
    Optional<BxDFSample> sample(const float3 wo, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);
    float3 f(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);
    float pdf(const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);

    EBxDFFlags flags();
};

[anyValueSize(60)]
interface IMaterial
{
    associatedtype MaterialInstance : IMaterialInstance;
    MaterialInstance getMaterialInstance(const float2 uv);
};

[anyValueSize(4)]
interface IAdapter
{
    Optional<BxDFSample> sample(const IMaterialInstance bsdf, const float3 wo, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);
    float3 f(const IMaterialInstance bsdf, const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance);
    float pdf(const IMaterialInstance bsdf, const float3 wo, const float3 wi, const ETransportMode mode = ETransportMode::radiance, const EBxDFSampleFlags sampleFlags = EBxDFSampleFlags::all);
};
