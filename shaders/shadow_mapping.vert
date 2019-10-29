#version 460

layout (location = 0) in vec3 aPosition;

layout (location = 1) uniform mat4 uProjMat;
layout (location = 2) uniform mat4 uViewMat;
layout (location = 3) uniform mat4 uModelMat;

void main()
{
    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPosition, 1);
}
