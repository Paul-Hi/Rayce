import globals;

[shader("closesthit")]
void main(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    payload.hit = true;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.hitKind = EHitKind::triangleMesh;
    payload.primitiveId = PrimitiveIndex();
    payload.instanceCustomIndex = InstanceID();
    payload.hitAttributes = attributes.barycentrics;
    payload.worldToObject = WorldToObject4x3();
    //payload.objectToWorld = ObjectToWorld4x3();
}
