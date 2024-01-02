import globals;

[shader("miss")]
void main(inout RayPayload payload)
{
    payload.hitKind = EHitKind::miss;
}
