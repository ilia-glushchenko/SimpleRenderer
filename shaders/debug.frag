#version 460

layout (location = 20, binding = 0) uniform sampler2D uLightingColorTextureSampler2D;
layout (location = 21, binding = 1) uniform sampler2D uLightingDepthTextureSampler2D;
layout (location = 22, binding = 2) uniform sampler2D uTaaHistoryTextureSampler2D;
layout (location = 23, binding = 3) uniform sampler2D uTaaDrawTextureSampler2D;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 outColor;

void main()
{
    vec3 lightingColor = texture(uLightingColorTextureSampler2D, uv).rgb;
    vec3 taaHistory = texture(uTaaHistoryTextureSampler2D, uv).rgb;
    vec3 taaColor = texture(uTaaDrawTextureSampler2D, uv).rgb;

    outColor = distance(lightingColor, taaColor) > 0.1f
        ? vec4(1, 0, 0, 1) : vec4(0, 1, 0.5, 1);

    outColor = texture(uTaaDrawTextureSampler2D, uv).rgba;
}
