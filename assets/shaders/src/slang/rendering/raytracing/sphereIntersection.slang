import core.globals;
import core.data;

struct SphereIntersection
{
    float t;
};

SphereIntersection intersectSphere(const Sphere s, const float3 origin, const float3 direction)
{
    SphereIntersection sphereIntersection;
    const float3 d = origin - s.center;
    const float a = dot(direction, direction);
    const float b = 2.0 * dot(d, direction);
    const float c = dot(d, d) - s.radius * s.radius;
    const float discriminant = b * b - 4.0 * a * c;
    sphereIntersection.t = -1.0;
    if(discriminant < 0)
    {
        return sphereIntersection;
    }
    else if (discriminant == 0)
    {
        sphereIntersection.t = -0.5 * b / a;
        return sphereIntersection;
    }
    else
    {
        float q = (b > 0) ? -0.5 * (b + sqrt(discriminant)) : -0.5 * (b - sqrt(discriminant));
        float t0 = q / a;
        float t1 = c / q;
        sphereIntersection.t = min(t0, t1);
        if(sphereIntersection.t > 0)
        {
            return sphereIntersection;
        }

        sphereIntersection.t = max(t0, t1);
        return sphereIntersection;
    }
}

[shader("intersection")]
void main()
{
    Sphere sphere = gSpheres[PrimitiveIndex()];

    uint hitKind = (uint)EHitKind::proceduralSphere; // FIXME: Always that atm.

    SphereIntersection sphereIntersection = intersectSphere(sphere, WorldRayOrigin(), WorldRayDirection());
    if(sphereIntersection.t >= 0.0)
    {
        ReportHit(sphereIntersection.t, hitKind, new SphereIntersectionAttributes());
    }
}
