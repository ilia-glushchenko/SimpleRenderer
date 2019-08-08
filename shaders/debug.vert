#version 460

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 uv;

void main()
{
    gl_Position = vec4(aPosition, 1);
    position = aPosition;
    normal = aNormal;
    uv = aUV;
}
