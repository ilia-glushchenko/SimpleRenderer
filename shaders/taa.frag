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

vec2 SampleLocal3x3DepthMinimaUV(sampler2D textureSampler, vec2 currentUV)
{
    ivec2 wh = textureSize(textureSampler, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec2 minimaUV = currentUV;
    float minimaDepth = 0;
    for (uint y = -1; y < 2; ++y) {
        for (uint x = -1; x < 2; ++x) {
            vec2 sampleUV = vec2(currentUV.x + x * dx, currentUV.y + y * dy);
            float sampleDepth = texture(textureSampler, sampleUV).r;
            minimaUV = sampleDepth < minimaDepth ? sampleUV : minimaUV;
            minimaDepth = min(minimaDepth, sampleDepth);
        }
    }

    return minimaUV;
}

vec3 SampleLocal3x3Minima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 minima =
        min(texture(tex, uv + vec2(-dx,-dy)),
        min(texture(tex, uv + vec2(  0,-dy)),
        min(texture(tex, uv + vec2( dx,-dy)),
        min(texture(tex, uv + vec2(-dx,  0)),
        min(texture(tex, uv + vec2(  0,  0)),
        min(texture(tex, uv + vec2( dx,  0)),
        min(texture(tex, uv + vec2(-dx, dy)),
        min(texture(tex, uv + vec2(  0, dy)),
            texture(tex, uv + vec2( dx, dy)))
    )))))));

    return minima.xyz;
}

vec3 SampleLocal3x3Maxima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 minima =
        max(texture(tex, uv + vec2(-dx,-dy)),
        max(texture(tex, uv + vec2(  0,-dy)),
        max(texture(tex, uv + vec2( dx,-dy)),
        max(texture(tex, uv + vec2(-dx,  0)),
        max(texture(tex, uv + vec2(  0,  0)),
        max(texture(tex, uv + vec2( dx,  0)),
        max(texture(tex, uv + vec2(-dx, dy)),
        max(texture(tex, uv + vec2(  0, dy)),
            texture(tex, uv + vec2( dx, dy)))
    )))))));

    return minima.xyz;
}

vec3 SampleLocalCrossMinima(sampler2D textureSampler, vec2 currentUV)
{
    ivec2 wh = textureSize(textureSampler, 0);

    float uTex = currentUV.x * wh.x;
    float vTex = currentUV.y * wh.y;

    vec4 minima = min(
        texture(textureSampler, currentUV),
        min(texture(textureSampler, vec2((uTex + 1) / wh.x, vTex / wh.y)),
            min(texture(textureSampler, vec2((uTex - 1) / wh.x, vTex / wh.y)),
                min(texture(textureSampler, vec2(uTex / wh.x, (vTex + 1) / wh.y)),
                    texture(textureSampler, vec2(uTex / wh.x, (vTex - 1) / wh.y))
    ))));

    return minima.xyz;
}

vec3 SampleLocalCrossMaxima(sampler2D textureSampler, vec2 currentUV)
{
    ivec2 wh = textureSize(textureSampler, 0);

    float uTex = currentUV.x * wh.x;
    float vTex = currentUV.y * wh.y;

    vec4 maxima = max(
        texture(textureSampler, currentUV),
        max(texture(textureSampler, vec2((uTex + 1) / wh.x, vTex / wh.y)),
            max(texture(textureSampler, vec2((uTex - 1) / wh.x, vTex / wh.y)),
                max(texture(textureSampler, vec2(uTex / wh.x, (vTex + 1) / wh.y)),
                    texture(textureSampler, vec2(uTex / wh.x, (vTex - 1) / wh.y))
    ))));

    return maxima.xyz;
}

vec3 ClipAABB(vec3 aabbMin, vec3 aabbMax, vec3 history, vec3 color)
{
    vec3 p_clip = 0.5 * (aabbMax + aabbMin);
    vec3 e_clip = 0.5 * (aabbMax - aabbMin);

    vec3 v_clip = history - p_clip;
    vec3 v_unit = v_clip.xyz / e_clip;
    vec3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return p_clip + v_clip / ma_unit;
    else
        return color;
}

void Velocity()
{
    vec2 jitter = vec2(uJitterMat[0].z, uJitterMat[1].z);
    vec2 reprojectedUV = ReverseReprojectUV(uv);
    vec3 color = texture(uColorTextureSampler2D, uv - jitter).rgb;
    outColor = vec4(color, 1);

    vec2 minVelUV = SampleLocal3x3DepthMinimaUV(uVelocityTextureSampler2D, uv);
    vec2 vel = texture(uVelocityTextureSampler2D, minVelUV).xy;
    vec2 velUV = uv - vel;
    if (velUV.x >= 0 && velUV.x <= 1 && velUV.y >= 0 && velUV.y <= 1)
    {
        vec3 minima = SampleLocal3x3Minima(uColorTextureSampler2D, velUV);
        vec3 maxima = SampleLocal3x3Maxima(uColorTextureSampler2D, velUV);

        vec3 history = texture(uHistoryTextureSampler2D, reprojectedUV).rgb;
        //history = ClipAABB(minima, maxima, history, color);
        history = clamp(history, minima, maxima);

        outColor = vec4(mix(history, color, 1.0/16.0), 1);
    }
}

void NoVelocity()
{
    vec2 jitter = vec2(uJitterMat[0].z, uJitterMat[1].z);
    vec2 reprojectedUV = ReverseReprojectUV(uv);
    vec3 color = texture(uColorTextureSampler2D, uv - jitter).rgb;

    vec3 minima = SampleLocalCrossMinima(uColorTextureSampler2D, uv - jitter);
    vec3 maxima = SampleLocalCrossMaxima(uColorTextureSampler2D, uv - jitter);

    vec3 history = texture(uHistoryTextureSampler2D, reprojectedUV).rgb;
    history = clamp(history, minima, maxima);

    outColor = vec4(mix(history, color, 1.0/128.0), 1);
}

vec3 SampleMinima(vec2 uv, vec2 jitter)
{
    vec3 minimaCross = SampleLocalCrossMinima(uColorTextureSampler2D, uv - jitter);
    vec3 minima3x3 = SampleLocal3x3Minima(uColorTextureSampler2D, uv - jitter);
    return mix(minimaCross, minima3x3, 0.5);
}

vec3 SampleMaxima(vec2 uv, vec2 jitter)
{
    vec3 maxima3x3 = SampleLocal3x3Maxima(uColorTextureSampler2D, uv - jitter);
    vec3 maximaCross = SampleLocalCrossMaxima(uColorTextureSampler2D, uv - jitter);
    return mix(maximaCross, maxima3x3, 0.5);
}

vec2 SampleVelocity(vec2 uv, vec2 jitter)
{
    vec2 velocityUV = SampleLocal3x3DepthMinimaUV(uVelocityTextureSampler2D, uv);
    vec2 velocity = texture(uVelocityTextureSampler2D, velocityUV).xy;

    return velocity;
}

void main()
{
    if (uFrameCountUint >= 2)
    {
        vec2 jitter = vec2(uJitterMat[0].z, uJitterMat[1].z);

        vec2 reprojectedUV = ReverseReprojectUV(uv);
        vec2 velocity = SampleVelocity(uv, jitter);

        vec3 history = texture(uHistoryTextureSampler2D, reprojectedUV - velocity).rgb;

        history = clamp(history, SampleMinima(uv, jitter), SampleMaxima(uv, jitter));

        float feedback = 1.0/80.0;
        vec3 color = texture(uColorTextureSampler2D, uv - jitter).rgb;
        outColor = vec4(mix(history, color, feedback), 1);
    }
    else
    {
        outColor = vec4(texture(uColorTextureSampler2D, uv).rgb, 1);
    }
}


