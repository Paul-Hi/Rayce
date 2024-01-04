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
