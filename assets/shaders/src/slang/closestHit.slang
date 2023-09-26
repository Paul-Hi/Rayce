import globals;

[shader("closesthit")]
void cHitTriangle(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    payload.hit = true;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.hitInfo.hitKind = EHitKind::triangleMesh;
    payload.hitInfo.primitiveId = PrimitiveIndex();
    payload.hitInfo.instanceCustomIndex = InstanceID();
    payload.hitInfo.hitAttributes = attributes.barycentrics;
    payload.hitInfo.worldToObject = WorldToObject4x3();
    payload.hitInfo.objectToWorld = ObjectToWorld4x3();
}

[shader("closesthit")]
void cHitSphere(inout RayPayload payload, in SphereIntersectionAttributes attributes)
{
    payload.hit = true;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.hitInfo.hitKind = EHitKind::proceduralSphere;
    payload.hitInfo.primitiveId = PrimitiveIndex();
    payload.hitInfo.instanceCustomIndex = InstanceID();
    payload.hitInfo.hitAttributes = attributes.inside ? float2(-1.0) : float2(1.0);
    payload.hitInfo.worldToObject = WorldToObject4x3();
    payload.hitInfo.objectToWorld = ObjectToWorld4x3();
}
