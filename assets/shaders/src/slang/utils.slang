import globals;

// tangent space trigonometry
float cosThetaTS(in float3 w) { return w.z; }
float cos2ThetaTS(in float3 w) { return w.z * w.z; }
float sin2ThetaTS(in float3 w) { return max(0.0, 1.0 - cos2ThetaTS(w)); }
float sinThetaTS(in float3 w) { return sqrt(sin2ThetaTS(w)); }
float tanThetaTS(in float3 w) { return sinThetaTS(w) / cosThetaTS(w); }
float tan2ThetaTS(in float3 w) { return sin2ThetaTS(w) / cos2ThetaTS(w); }
float cosPhiTS(in float3 w) { return (sinThetaTS(w) == 0.0) ? 1.0 : clamp(w.x / sinThetaTS(w), -1.0, 1.0); }
float sinPhiTS(in float3 w) { return (sinThetaTS(w) == 0.0) ? 0.0 : clamp(w.y / sinThetaTS(w), -1.0, 1.0); }
float cos2PhiTS(in float3 w) { return cosPhiTS(w) * cosPhiTS(w); }
float sin2PhiTS(in float3 w) { return sinPhiTS(w) * sinPhiTS(w); }

float3 reflectTS(in float3 w) { return float3(-w.x, -w.y, w.z); }
float3 refractTS(in float3 w, in float cosThetaT, in float etaTI)
{
    return float3(-etaTI * w.x, -etaTI * w.y, cosThetaT);
}

// similar to mitsuba
float hypot2(float a, float b)
{
    float r;
    if (abs(a) > abs(b))
    {
        r = b / a;
        r = abs(a) * sqrt(1.0 + r * r);
    }
    else if (b != 0.0f)
    {
        r = a / b;
        r = abs(b) * sqrt(1.0 + r * r);
    }
    else
    {
        r = 0.0;
    }

    return r;
}

func safe_sqrt<T : __BuiltinFloatingPointType>(T v) -> T
{
    return sqrt(max(T(0.0), v));
}

func safe_sqrt<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> v) -> vector<T, N>
{
    return sqrt(max(vector<T, N>(T(0.0)), v));
}

func safe_sqrt<T : __BuiltinFloatingPointType, let N : int, let M : int>(matrix<T, N, M> v) -> matrix<T, N, M>
{
    return sqrt(max(matrix<T, N, M>(T(0.0)), v));
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

float3 reflect(const float3 wi, const float3 n)
{
    return 2.0 * dot(wi, n) * n - wi;
}

float3 refract(const float3 wi, const float3 n, float cosThetaT, float eta)
{
    if (cosThetaT < 0.0)
    {
        eta = 1.0 / eta;
    }

    return n * (dot(wi, n) * eta + cosThetaT) - wi * eta;
}

struct CoordinateSpace
{
    __init(const float3 normal, bool hasUV = false,
                                      const float3 dfd1 = float3(0.0), const float3 dfd2 = float3(0.0),
                                      const float2 uvd1 = float2(0.0), const float2 uvd2 = float2(0.0)
                                      )
    {
        this.normal = normal;
        if(hasUV && uvd1.x > 0.0)
        {
            float3 t_    = (uvd2.y * dfd1 - uvd1.y * dfd2) / (uvd1.x * uvd2.y - uvd2.x * uvd1.y);
            this.tangent    = normalize(t_ - normal * dot(normal, t_));
            this.bitangent  = cross(normal, this.tangent);
        }
        else
        {
            // Not consistent for spheres
            // Building an Orthonormal Basis, Revisited, (JCGT), vol. 6, no. 1, 1-8, 2017
            // Available online: http://jcgt.org/published/0006/01/01/
            // float normalSign = sign(normal.z);
            // float a = -1.0 / (normalSign + normal.z);
            // float b = normal.x * normal.y * a;
            // this.tangent = float3(1.0 + normalSign * normal.x * normal.x * a, normalSign * b, -normalSign * normal.x);
            // this.bitangent = float3(b, normalSign + normal.y * normal.y * a, -normal.y);

            float3 a = (abs(normal.x) > 0.9999) ? float3(0,1,0) : float3(1,0,0);
            this.bitangent = normalize(cross(normal, a));
            this.tangent = cross(normal, this.bitangent);

            // this.tangent = normalize(float3(-normal.y, normal.x, 0.0));
            // this.bitangent = cross(normal, this.tangent);
        }
    }

    CoordinateSpace flipped()
    {
        CoordinateSpace flippedSpace;
        flippedSpace.normal = -this.normal;
        flippedSpace.tangent = -this.tangent;
        flippedSpace.bitangent = -this.bitangent;
        return flippedSpace;
    }

    float3 tangentToWorld(const float3 p)
    {
        return multiplyAndAdd(float3(p.x), tangent, multiplyAndAdd(float3(p.y), bitangent, p.z * normal));
    }

    float3 worldToTangentFrame(in float3 p)
    {
        return float3(dot(p, tangent), dot(p, bitangent), dot(p, normal));
    }

    float3 normal;
    float3 tangent;
    float3 bitangent;
};

// beta = 2 - number samples = 1
float powerHeuristic(float fPdf, float gPdf)
{
    fPdf *= fPdf;
    gPdf *= gPdf;
    return fPdf / (fPdf + gPdf);
}

float2 mapToConcentricDisc(const float2 p)
{
    float2 offset = p * 2.0 - 1.0;

    if(offset.x == 0 && offset.y == 0)
    {
        return float2(0.0);
    }

    float theta, r;
    if (abs(offset.x) > abs(offset.y))
    {
        r = offset.x;
        theta = QUARTER_PI * (offset.y / offset.x);
    }
    else
    {
        r = offset.y;
        theta = HALF_PI - QUARTER_PI * (offset.x / offset.y);
    }
    return r * float2(cos(theta), sin(theta));
}

static const float GAMMA     = 2.2;
static const float INV_GAMMA = 1.0 / GAMMA;

float3 sRGBToLinear(in float3 srgb)
{
    return pow(srgb.rgb, float3(GAMMA));
}

float3 linearTosRGB(in float3 linear)
{
    return pow(linear.rgb, float3(INV_GAMMA));
}

// ACES tone map
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 toneMapACES(in float3 color)
{
    float A = 2.51;
    float B = 0.03;
    float C = 2.43;
    float D = 0.59;
    float E = 0.14;
    return saturate((color * (A * color + B)) / (color * (C * color + D) + E));
}
