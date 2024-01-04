import core.globals;

[shader("miss")]
void main(inout ShadowRayPayload payload)
{
    payload.hit = false;
}
