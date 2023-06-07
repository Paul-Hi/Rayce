#version 450

layout(location = 0) in vec3 perVertexColor;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(perVertexColor, 1.0);
}
