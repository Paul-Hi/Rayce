import globals;

[shader("closesthit")]
void main(inout RayPayload payload, in SphereIntersectionAttributes attributes)
{
    payload.hit = true;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.hitKind = EHitKind::proceduralSphere;
    payload.primitiveId = PrimitiveIndex();
    payload.instanceCustomIndex = InstanceID();
    payload.worldToObject = WorldToObject4x3();
    //payload.objectToWorld = ObjectToWorld4x3();
}
