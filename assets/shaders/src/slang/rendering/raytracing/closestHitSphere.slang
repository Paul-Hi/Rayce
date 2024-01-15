import core.globals;
import core.data;

[shader("closesthit")]
void main(inout RayPayload payload, in SphereIntersectionAttributes attributes)
{
    payload.hitKind = EHitKind::proceduralSphere;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    const DeviceSphere sphere = getSphere(payload.hitPoint, PrimitiveIndex(), InstanceID(), WorldToObject4x3());
    payload.materialId = sphere.materialId;
    payload.uv = sphere.uv;
    payload.lightId = sphere.lightId;
    payload.space = CoordinateSpace(sphere.normal, true);
}
