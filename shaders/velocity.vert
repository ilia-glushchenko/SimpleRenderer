#version 460

layout (location = 10) uniform mat4 uPrevModelMat4;
layout (location = 11) uniform mat4 uPrevViewMat4;
layout (location = 12) uniform mat4 uPrevProjMat4;
layout (location = 13) uniform mat4 uModelMat4;
layout (location = 14) uniform mat4 uViewMat4;
layout (location = 15) uniform mat4 uProjMat4;

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

layout (location = 0) out vec4 prevPosition;
layout (location = 1) out vec4 position;

void main()
{
    mat4 prevMVP = uPrevProjMat4 * uPrevViewMat4 * uPrevModelMat4;
    mat4 MVP = uProjMat4 * uViewMat4 * uModelMat4;

    prevPosition = prevMVP * vec4(aPosition, 1);
    position = MVP * vec4(aPosition, 1);

    gl_Position = position;
}