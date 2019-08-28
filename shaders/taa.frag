#version 460

layout (location = 10) uniform mat4 uViewMat;
layout (location = 11) uniform mat4 uProjMat;
layout (location = 12) uniform mat4 uJitterMat;
layout (location = 13) uniform mat4 uPrevJitterMat;
layout (location = 14) uniform mat4 uPrevViewMat;
layout (location = 15) uniform mat4 uPrevProjMat;
layout (location = 16) uniform uint uFrameCountUint;

layout (location = 20, binding = 0) uniform sampler2D uColorTextureSampler2D;
layout (location = 21, binding = 1) uniform sampler2D uDepthTextureSampler2D;
layout (location = 22, binding = 2) uniform sampler2D uHistoryTextureSampler2D;
layout (location = 23, binding = 3) uniform sampler2D uVelocityTextureSampler2D;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 outColor;

vec3 WorldPosFromDepth(float depth, vec2 TexCoord)
{
    vec4 clipSpacePosition = vec4(TexCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = inverse(uProjMat) * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = inverse(uViewMat) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec3 ReverseReprojectFrag(vec2 uv, vec2 jitter)
{
    vec3 fragPosWorld = WorldPosFromDepth(texture(uDepthTextureSampler2D, uv - jitter).r, uv);

    vec4 fragPosPrevProj = uPrevProjMat * uPrevViewMat * vec4(fragPosWorld, 1);

    return fragPosPrevProj.xyz / fragPosPrevProj.w;
}

vec2 SampleLocal3x3DepthMinimaUV(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec2 minimaUV = uv;
    float minimaDepth = texture(tex, minimaUV).r;
    for (uint y = -1; y < 2; ++y)
    {
        for (uint x = -1; x < 2; ++x)
        {
            vec2 sampleUV = uv + vec2(x*dx, y*dy);
            float sampleDepth = texture(tex, sampleUV).r;

            minimaUV = sampleDepth < minimaDepth ? sampleUV : minimaUV;
            minimaDepth = min(sampleDepth, minimaDepth);
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

vec3 SampleLocalCrossMinima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 minima = min(
        texture(tex, uv),
        min(texture(tex, uv + vec2( dx,  0)),
        min(texture(tex, uv + vec2(  0, dy)),
        min(texture(tex, uv + vec2(-dx,  0)),
            texture(tex, uv + vec2(  0,-dy))
    ))));

    return minima.xyz;
}

vec3 SampleLocalCrossMaxima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 maxima = min(
        texture(tex, uv),
        max(texture(tex, uv + vec2( dx,  0)),
        max(texture(tex, uv + vec2(  0, dy)),
        max(texture(tex, uv + vec2(-dx,  0)),
            texture(tex, uv + vec2(  0,-dy))
    ))));

    return maxima.xyz;
}

vec3 SampleMinima(vec2 uv)
{
    vec3 minimaCross = SampleLocalCrossMinima(uColorTextureSampler2D, uv);
    vec3 minima3x3 = SampleLocal3x3Minima(uColorTextureSampler2D, uv);
    return mix(minimaCross, minima3x3, 0.5);
}

vec3 SampleMaxima(vec2 uv)
{
    vec3 maxima3x3 = SampleLocal3x3Maxima(uColorTextureSampler2D, uv);
    vec3 maximaCross = SampleLocalCrossMaxima(uColorTextureSampler2D, uv);
    return mix(maximaCross, maxima3x3, 0.5);
}

vec2 SampleVelocity(vec2 uv)
{
    vec2 velocityUV = SampleLocal3x3DepthMinimaUV(uDepthTextureSampler2D, uv);
    vec2 velocity = texture(uVelocityTextureSampler2D, velocityUV).xy;

    return velocity;
}

// https://software.intel.com/en-us/node/503873
vec3 RGB2YCoCg(vec3 c)
{
    // Y = R/4 + G/2 + B/4
    // Co = R/2 - B/2
    // Cg = -R/4 + G/2 - B/4
    return vec3(
            c.x/4.0 + c.y/2.0 + c.z/4.0,
            c.x/2.0 - c.z/2.0,
        -c.x/4.0 + c.y/2.0 - c.z/4.0
    );
}

// https://software.intel.com/en-us/node/503873
vec3 YCoCg2RGB(vec3 c)
{
    // R = Y + Co - Cg
    // G = Y + Cg
    // B = Y - Co - Cg
    return clamp(vec3(
        c.x + c.y - c.z,
        c.x + c.z,
        c.x - c.y - c.z
    ), 0, 1);
}

vec3 ClipAABB(vec3 aabb_min, vec3 aabb_max, vec3 p, vec3 q)
{
    aabb_min = RGB2YCoCg(aabb_min);
    aabb_max = RGB2YCoCg(aabb_max);
    p = RGB2YCoCg(p);
    q = RGB2YCoCg(q);

    const float FLT_EPS = 0.00000001f;

    vec3 r = q - p;
    vec3 rmax = aabb_max - p.xyz;
    vec3 rmin = aabb_min - p.xyz;

    const float eps = FLT_EPS;

    if (r.x > rmax.x + eps)
        r *= (rmax.x / r.x);
    if (r.y > rmax.y + eps)
        r *= (rmax.y / r.y);
    if (r.z > rmax.z + eps)
        r *= (rmax.z / r.z);

    if (r.x < rmin.x - eps)
        r *= (rmin.x / r.x);
    if (r.y < rmin.y - eps)
        r *= (rmin.y / r.y);
    if (r.z < rmin.z - eps)
        r *= (rmin.z / r.z);

    return YCoCg2RGB((p + r).xyz);
}

void main()
{
    const float feedback = 1.0/16.0;
    //vec2 jitter = vec2(uJitterMat[3][0], uProjMat[3][1]);
    vec2 jitter = vec2(0);

    if (uFrameCountUint >= 2)
    {
        vec3 fragPosPrevProj = ReverseReprojectFrag(uv, jitter);
        if (fragPosPrevProj.x > 1 || fragPosPrevProj.x < -1 || fragPosPrevProj.y > 1 || fragPosPrevProj.y < -1)
        {
            //ToDo: Gaussian Blur
            outColor = vec4(texture(uColorTextureSampler2D, uv).rgb, 1);
        }
        else
        {
            vec2 prevFrameUV = vec2(
                0.5 * fragPosPrevProj.x + 0.5,
                0.5 * fragPosPrevProj.y + 0.5);

            vec2 historyUV = prevFrameUV;
            //vec2 historyUV = uv - SampleVelocity(uv - jitter);

            if (historyUV.x >= 0 && historyUV.x <= 1 && historyUV.y >= 0 && historyUV.y <= 1)
            {
                vec3 history = clamp(texture(uHistoryTextureSampler2D, historyUV).rgb,
                    SampleMinima(uv - jitter), SampleMaxima(uv - jitter));
                //history = texture(uHistoryTextureSampler2D, historyUV).rgb;
                vec3 color = texture(uColorTextureSampler2D, uv - jitter).rgb;
                outColor = vec4(mix(history, color, feedback), 1);
            }
            else
            {
                //ToDo: Gaussian Blur
                outColor = vec4(texture(uColorTextureSampler2D, uv).rgb, 1);
            }
        }
    }
    else
    {
        outColor = vec4(texture(uColorTextureSampler2D, uv).rgb, 1);
    }
}

