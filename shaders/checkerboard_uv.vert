#version 460

layout (location = 10) uniform mat4 uModelMat;
layout (location = 11) uniform mat4 uViewMat;
layout (location = 12) uniform mat4 uProjMat;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec2 outUV;

void main()
{
    gl_Position = uProjMat * uViewMat * uModelMat * vec4(inPosition, 1);
    outUV = inUV;
}