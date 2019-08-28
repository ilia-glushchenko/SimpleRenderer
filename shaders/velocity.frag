#version 460

layout (location = 10) uniform mat4 uPrevModelMat4;
layout (location = 11) uniform mat4 uPrevViewMat4;
layout (location = 12) uniform mat4 uPrevProjMat4;
layout (location = 13) uniform mat4 uModelMat4;
layout (location = 14) uniform mat4 uViewMat4;
layout (location = 15) uniform mat4 uProjMat4;

layout (location = 0) in vec4 inPrevPosition;
layout (location = 1) in vec4 inPosition;

layout (location = 0) out vec4 outVelocity;

void main()
{
    vec3 currentPos = inPosition.xyz / inPosition.w;
    vec3 prevPos = inPrevPosition.xyz / inPrevPosition.w;
    vec3 velocity = (currentPos - prevPos) / 2;

    outVelocity = vec4(velocity, 1);
}