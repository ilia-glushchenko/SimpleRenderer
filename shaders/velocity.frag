#version 460

layout (location = 0) in vec4 inPrevPosition;
layout (location = 1) in vec4 inPosition;

layout (location = 0) out vec4 outVelocity;

bool IsNan(vec3 color)
{
    return isnan(color.x) || isnan(color.y) || isnan(color.z);
}

void main()
{
    vec3 currentPos = inPosition.xyz / inPosition.w;
    vec3 prevPos = inPrevPosition.xyz / inPrevPosition.w;
    vec3 velocity = (currentPos - prevPos) / 2;

    outVelocity = vec4(0, 0, 0, 1);

    if (length(velocity) > 1e-5f)
    {
        outVelocity = vec4(velocity, 1);
    }
}