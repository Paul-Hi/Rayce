import core.globals;
import core.data;

[shader("closesthit")]
void main(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{;
    payload.hitKind = EHitKind::triangleMesh;
    payload.hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    const DeviceTriangle triangle = getTriangle(PrimitiveIndex(), InstanceID(), attributes.barycentrics, WorldToObject4x3());
    payload.materialId = triangle.materialId;
    payload.uv = triangle.interpolatedUV;
    payload.lightId = triangle.lightId;
    payload.space = CoordinateSpace(!any(abs(triangle.interpolatedNormal) > 0.0) ? triangle.geometryNormal : triangle.interpolatedNormal,
                                        gMaterials[triangle.materialId].canUseUv == 1, triangle.dfd1, triangle.dfd2, triangle.uvd1, triangle.uvd2);
}
