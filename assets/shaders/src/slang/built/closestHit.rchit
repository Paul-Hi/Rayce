#version 460
#extension GL_EXT_ray_tracing : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 42 0
struct Hit_0
{
    uint hitKind_0;
    uint primitiveId_0;
    uint instanceCustomIndex_0;
    vec2 hitAttributes_0;
    mat4x3 worldToObject_0;
    mat4x3 objectToWorld_0;
};

struct RayPayload_0
{
    bool hit_0;
    vec3 hitPoint_0;
    Hit_0 hitInfo_0;
};


#line 4 1
rayPayloadInEXT RayPayload_0 _S1;


#line 7230 2
struct BuiltInTriangleIntersectionAttributes_0
{
    vec2 barycentrics_0;
};


#line 7230
hitAttributeEXT BuiltInTriangleIntersectionAttributes_0 _S2;


#line 4 1
void main()
{
    _S1.hit_0 = true;
    vec3 _S3 = ((gl_WorldRayOriginEXT));

#line 7
    vec3 _S4 = ((gl_WorldRayDirectionEXT));

#line 7
    float _S5 = ((gl_RayTmaxEXT));

#line 7
    _S1.hitPoint_0 = _S3 + _S4 * _S5;
    _S1.hitInfo_0.hitKind_0 = 0U;
    uint _S6 = ((gl_PrimitiveID));

#line 9
    _S1.hitInfo_0.primitiveId_0 = _S6;
    uint _S7 = ((gl_InstanceCustomIndexEXT));

#line 10
    _S1.hitInfo_0.instanceCustomIndex_0 = _S7;
    _S1.hitInfo_0.hitAttributes_0 = _S2.barycentrics_0;
    mat4x3 _S8 = (transpose(gl_WorldToObject3x4EXT));

#line 12
    _S1.hitInfo_0.worldToObject_0 = _S8;
    mat4x3 _S9 = (transpose(gl_ObjectToWorld3x4EXT));

#line 13
    _S1.hitInfo_0.objectToWorld_0 = _S9;
    return;
}

