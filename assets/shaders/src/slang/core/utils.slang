__exported import core.globals;

// tangent space trigonometry
float cosThetaTS(const float3 w) { return w.z; }
float cos2ThetaTS(const float3 w) { return w.z * w.z; }
float sin2ThetaTS(const float3 w) { return max(0.0, 1.0 - cos2ThetaTS(w)); }
float sinThetaTS(const float3 w) { return sqrt(sin2ThetaTS(w)); }
float tanThetaTS(const float3 w) { return sinThetaTS(w) / cosThetaTS(w); }
float tan2ThetaTS(const float3 w) { return sin2ThetaTS(w) / cos2ThetaTS(w); }
float cosPhiTS(const float3 w) { return (sinThetaTS(w) == 0.0) ? 1.0 : clamp(w.x / sinThetaTS(w), -1.0, 1.0); }
float sinPhiTS(const float3 w) { return (sinThetaTS(w) == 0.0) ? 0.0 : clamp(w.y / sinThetaTS(w), -1.0, 1.0); }
float cos2PhiTS(const float3 w) { return cosPhiTS(w) * cosPhiTS(w); }
float sin2PhiTS(const float3 w) { return sinPhiTS(w) * sinPhiTS(w); }

func sqr<T : __BuiltinFloatingPointType>(T v) -> T
{
    return v * v;
}

func sqr<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> v) -> vector<T, N>
{
    return v * v;
}

func sqr<T : __BuiltinFloatingPointType, let N : int, let M : int>(matrix<T, N, M> v) -> matrix<T, N, M>
{
    return v * v;
}

func safe_sqrt<T : __BuiltinFloatingPointType>(T v) -> T
{
    return sqrt(max(T(1e-9), v)); // looks like slang sqrt(0) is nan?
}

func safe_sqrt<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> v) -> vector<T, N>
{
    return sqrt(max(vector<T, N>(T(1e-9)), v)); // looks like slang sqrt(0) is nan?
}

func safe_sqrt<T : __BuiltinFloatingPointType, let N : int, let M : int>(matrix<T, N, M> v) -> matrix<T, N, M>
{
    return sqrt(max(matrix<T, N, M>(T(1e-9)), v)); // looks like slang sqrt(0) is nan?
}

func multiplyAndAdd<T : __BuiltinFloatingPointType>(T a, T b, T c) -> T
{
    return a * b + c;
}

func multiplyAndAdd<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> a, vector<T, N> b, vector<T, N> c) -> vector<T, N>
{
    return a * b + c;
}

func negativeMultiplyAndAdd<T : __BuiltinFloatingPointType>(T a, T b, T c) -> T
{
    return -a * b + c;
}

func negativeMultiplyAndAdd<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> a, vector<T, N> b, vector<T, N> c) -> vector<T, N>
{
    return -a * b + c;
}

// 32-bit bit interleave (Morton code) from Nvidias Falcor.
uint interleave32Bit(uint2 v)
{
    uint x = v.x & 0x0000ffff;
    x = (x | (x << 8)) & 0x00FF00FF;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;

    uint y = v.y & 0x0000ffff;
    y = (y | (y << 8)) & 0x00FF00FF;
    y = (y | (y << 4)) & 0x0F0F0F0F;
    y = (y | (y << 2)) & 0x33333333;
    y = (y | (y << 1)) & 0x55555555;

    return x | (y << 1);
}

float2 shirley(const float2 uv)
{
    return select(uv < 0.5, -0.5 + sqrt(2.0 * uv), 1.5 - sqrt(2.0 - 2.0 * uv));
}
