import core.globals;

[shader("closesthit")]
void main(inout RayPayload payload, in SphereIntersectionAttributes attributes)
{
    payload.hitKind = EHitKind::proceduralSphere;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.primitiveId = PrimitiveIndex();
    payload.instanceCustomIndex = InstanceID();
    payload.worldToObject = WorldToObject4x3();
    //payload.objectToWorld = ObjectToWorld4x3();
}
