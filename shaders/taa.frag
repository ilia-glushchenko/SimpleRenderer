#version 460

layout (location = 10) uniform mat4 uViewMat;
layout (location = 11) uniform mat4 uProjMat;
layout (location = 12) uniform mat4 uPrevViewMat;
layout (location = 13) uniform mat4 uPrevProjMat;

layout (location = 15) uniform vec3  uJitVec3;
layout (location = 16) uniform float uNearFloat;
layout (location = 17) uniform float uFarFloat;
layout (location = 18) uniform uint  uFrameCountUint;

layout (location = 20, binding = 0) uniform sampler2D uColorTextureSampler2D;
layout (location = 21, binding = 1) uniform sampler2D uDepthTextureSampler2D;
layout (location = 22, binding = 2) uniform sampler2D uHistoryTextureSampler2D;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 outColor;

vec3 SampleHistoryTexture()
{
    vec4 bottomLeftNDC = vec4(-1, 1, -1, 1);
    vec4 topRightNDC = vec4(1, -1, -1, 1);

    mat4 inverseProj = inverse(uProjMat);
    vec4 bottomLeftViewH = inverseProj * bottomLeftNDC;
    vec4 topRightViewH = inverseProj * topRightNDC;

    vec3 bottomLeft = bottomLeftViewH.xyz / bottomLeftViewH.w;
    vec3 topRight = topRightViewH.xyz / topRightViewH.w;

    float x = mix(bottomLeft.x, topRight.x, uv.x);
    float y = mix(topRight.y, bottomLeft.y, uv.y);
    float z = bottomLeft.z;
    float depth = texture(uDepthTextureSampler2D, uv).x;

    vec3 fragPosView = vec3(x, y, z) * mix(uNearFloat, uFarFloat, depth);
    vec3 fragPosWorld = (inverse(uViewMat) * vec4(fragPosView, 1)).xyz;

    vec4 fragPosPrevProj = uPrevProjMat * uPrevViewMat * vec4(fragPosWorld, 1);
    vec2 fragUV = vec2(
        0.5 * (fragPosPrevProj.x / fragPosPrevProj.w) + 0.5,
        0.5 * (fragPosPrevProj.y / fragPosPrevProj.w) + 0.5
    );

    vec3 prevColor = texture(uHistoryTextureSampler2D, fragUV).rgb;

    return prevColor;
}

void main()
{
    if (uFrameCountUint >= 2)
    {
        vec3 prevColor = SampleHistoryTexture();
        vec3 color = texture(uColorTextureSampler2D, uv - uJitVec3.xy).rgb;

        outColor = vec4(mix(color, prevColor, 0.7), 1);
    }
    else
    {
        outColor = vec4(texture(uColorTextureSampler2D, uv).rgb, 1);
    }
}
