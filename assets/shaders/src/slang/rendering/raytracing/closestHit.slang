import core.globals;

[shader("closesthit")]
void main(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{;
    payload.hitKind = EHitKind::triangleMesh;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.primitiveId = PrimitiveIndex();
    payload.instanceCustomIndex = InstanceID();
    payload.hitAttributes = attributes.barycentrics;
    payload.worldToObject = WorldToObject4x3();
    //payload.objectToWorld = ObjectToWorld4x3();
}
