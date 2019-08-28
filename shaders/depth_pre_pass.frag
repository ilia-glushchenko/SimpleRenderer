#version 460

layout (location = 0) in vec4 inPosition;
layout (location = 0) out float outDepth;

void main()
{
    outDepth = 1 - gl_FragCoord.z;
}