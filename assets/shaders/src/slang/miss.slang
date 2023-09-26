import globals;

[shader("miss")]
void main(inout RayPayload payload)
{
    payload.hit = false;
}
