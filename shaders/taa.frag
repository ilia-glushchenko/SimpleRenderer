#version 460

layout (location = 10) uniform mat4 uViewMat;
layout (location = 11) uniform mat4 uProjMat;
layout (location = 12) uniform mat4 uJitterMat;
layout (location = 13) uniform mat4 uPrevViewMat;
layout (location = 14) uniform mat4 uPrevProjMat;

layout (location = 16) uniform float uNearFloat;
layout (location = 17) uniform float uFarFloat;
layout (location = 18) uniform uint  uFrameCountUint;

layout (location = 20, binding = 0) uniform sampler2D uColorTextureSampler2D;
layout (location = 21, binding = 1) uniform sampler2D uDepthTextureSampler2D;
layout (location = 22, binding = 2) uniform sampler2D uHistoryTextureSampler2D;
layout (location = 23, binding = 3) uniform sampler2D uVelocityTextureSampler2D;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 outColor;

struct FrustrumCorners
{
    vec3 bottomLeft;
    vec3 topRight;
};

FrustrumCorners CalculateFrustrumCorners()
{
    vec4 bottomLeftNDC = vec4(-1, 1, -1, 1);
    vec4 topRightNDC = vec4(1, -1, -1, 1);

    mat4 inverseProj = inverse(uProjMat);
    vec4 bottomLeftViewH = inverseProj * bottomLeftNDC;
    vec4 topRightViewH = inverseProj * topRightNDC;

    vec3 bottomLeft = bottomLeftViewH.xyz / bottomLeftViewH.w;
    vec3 topRight = topRightViewH.xyz / topRightViewH.w;

    return FrustrumCorners(bottomLeft, topRight);
}

vec2 ReverseReprojectUV(vec2 currentUV)
{
    FrustrumCorners fc = CalculateFrustrumCorners();

    float x = mix(fc.bottomLeft.x, fc.topRight.x, currentUV.x);
    float y = mix(fc.topRight.y, fc.bottomLeft.y, currentUV.y);
    float z = fc.bottomLeft.z;
    float depth = texture(uDepthTextureSampler2D, currentUV).x;

    vec3 fragPosView = vec3(x, y, z) * mix(uNearFloat, uFarFloat, depth);
    vec3 fragPosWorld = (inverse(uViewMat) * vec4(fragPosView, 1)).xyz;

    vec4 fragPosPrevProj = uPrevProjMat * uPrevViewMat * vec4(fragPosWorld, 1);
    vec2 fragUV = vec2(
        0.5 * (fragPosPrevProj.x / fragPosPrevProj.w) + 0.5,
        0.5 * (fragPosPrevProj.y / fragPosPrevProj.w) + 0.5
    );

    return fragUV;
}

vec3 SampleLocalMinima(vec2 currentUV)
{
    ivec2 wh = textureSize(uColorTextureSampler2D, 0);

    float uTex = currentUV.x * wh.x;
    float vTex = currentUV.y * wh.y;

    vec4 minima = min(
        texture(uColorTextureSampler2D, currentUV),
        min(texture(uColorTextureSampler2D, vec2((uTex + 1) / wh.x, vTex / wh.y)),
            min(texture(uColorTextureSampler2D, vec2((uTex - 1) / wh.x, vTex / wh.y)),
                min(texture(uColorTextureSampler2D, vec2(uTex / wh.x, (vTex + 1) / wh.y)),
                    texture(uColorTextureSampler2D, vec2(uTex / wh.x, (vTex - 1) / wh.y))
                )
            )
        )
    );

    return minima.xyz;
}

vec3 SampleLocalMaxima(vec2 currentUV)
{
    ivec2 wh = textureSize(uColorTextureSampler2D, 0);

    float uTex = currentUV.x * wh.x;
    float vTex = currentUV.y * wh.y;

    vec4 maxima = max(
        texture(uColorTextureSampler2D, currentUV),
        max(texture(uColorTextureSampler2D, vec2((uTex + 1) / wh.x, vTex / wh.y)),
            max(texture(uColorTextureSampler2D, vec2((uTex - 1) / wh.x, vTex / wh.y)),
                max(texture(uColorTextureSampler2D, vec2(uTex / wh.x, (vTex + 1) / wh.y)),
                    texture(uColorTextureSampler2D, vec2(uTex / wh.x, (vTex - 1) / wh.y))
                )
            )
        )
    );

    return maxima.xyz;
}

void main()
{
    if (uFrameCountUint >= 2)
    {
        vec2 jitter = vec2(uJitterMat[0].z, uJitterMat[1].z);
        vec2 reprojectedUV = ReverseReprojectUV(uv);
        vec3 color = texture(uColorTextureSampler2D, uv - jitter).rgb;

        vec3 minima = SampleLocalMinima(uv - jitter);
        vec3 maxima = SampleLocalMaxima(uv - jitter);

        vec3 history = texture(uHistoryTextureSampler2D, reprojectedUV).rgb;
        history = clamp(history, minima, maxima);

        outColor = vec4(mix(history, color, 1.0/16.0), 1);
    }
    else
    {
        outColor = vec4(texture(uColorTextureSampler2D, uv).rgb, 1);
    }
}
