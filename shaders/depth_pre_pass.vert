#version 460

layout (location = 10) uniform mat4 uJitMat;
layout (location = 11) uniform mat4 uProjMat;
layout (location = 12) uniform mat4 uViewMat;
layout (location = 13) uniform mat4 uModelMat;

layout (location = 0) in vec3 aPosition;

void main()
{
    gl_Position = uJitMat * uProjMat * uViewMat * uModelMat * vec4(aPosition, 1);
}