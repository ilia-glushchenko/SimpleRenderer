#version 460

layout (location = 10) uniform mat4 uViewMat;
layout (location = 11) uniform mat4 uProjMat;
layout (location = 12) uniform mat4 uPrevViewMat;
layout (location = 13) uniform mat4 uPrevProjMat;
layout (location = 14) uniform uint uFrameCountUint;
layout (location = 15) uniform vec2 uJitterVec2;

layout (location = 20, binding = 0) uniform sampler2D uColorTextureSampler2D;
layout (location = 21, binding = 1) uniform sampler2D uDepthTextureSampler2D;
layout (location = 22, binding = 2) uniform sampler2D uHistoryTextureSampler2D;
layout (location = 23, binding = 3) uniform sampler2D uVelocityTextureSampler2D;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 outColor;

#define USE_YCoCg
#define USE_CLIPPING
#define USE_CLAMPING
//#define USE_REPROJECTED_HISTORY_UV
//#define USE_VELOCITY_CORRECTED_UV

float Linear2sRGB(float channel)
{
    if(channel <= 0.0031308)
        return 12.92 * channel;
    else
        return (1.0 + 0.055) * pow(channel, 1.0/2.4) - 0.055;
}

float SRGB2Linear(float channel)
{
    if (channel <= 0.04045)
        return channel / 12.92;
    else
        return pow((channel + 0.055) / (1.0 + 0.055), 2.4);
}

vec3 Linear2sRGB(vec3 color)
{
    return vec3(
        Linear2sRGB(color.r),
        Linear2sRGB(color.g),
        Linear2sRGB(color.b)
    );
}

vec3 SRGB2Linear(vec3 color)
{
    return vec3(
        SRGB2Linear(color.r),
        SRGB2Linear(color.g),
        SRGB2Linear(color.b)
    );
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
    for (uint y = -2; y < 3; ++y)
    {
        for (uint x = -2; x < 3; ++x)
        {
            vec2 sampleUV = uv + vec2(x*dx, y*dy);
            float sampleDepth = texture(tex, sampleUV).r;

            minimaUV = sampleDepth < minimaDepth ? sampleUV : minimaUV;
            minimaDepth = min(sampleDepth, minimaDepth);
        }
    }

    return minimaUV;
}

vec4 SampleColor(sampler2D tex, vec2 uv)
{
#ifdef USE_YCoCg
    vec4 color = texture(tex, uv);
    return vec4(RGB2YCoCg(color.rgb), color.a);
#else
    return texture(tex, uv);
#endif
}

vec4 ResolveColor(vec4 color)
{
#ifdef USE_YCoCg
    return vec4(YCoCg2RGB(color.xyz), color.a);
#else
    return color;
#endif
}

vec3 SampleLocal3x3Minima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 minima =
        min(SampleColor(tex, uv + vec2(-dx,-dy)),
        min(SampleColor(tex, uv + vec2(  0,-dy)),
        min(SampleColor(tex, uv + vec2( dx,-dy)),
        min(SampleColor(tex, uv + vec2(-dx,  0)),
        min(SampleColor(tex, uv + vec2( dx,  0)),
        min(SampleColor(tex, uv + vec2(-dx, dy)),
        min(SampleColor(tex, uv + vec2(  0, dy)),
            SampleColor(tex, uv + vec2( dx, dy)))
    ))))));

    return minima.xyz;
}

vec3 SampleLocal3x3Maxima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 minima =
        max(SampleColor(tex, uv + vec2(-dx,-dy)),
        max(SampleColor(tex, uv + vec2(  0,-dy)),
        max(SampleColor(tex, uv + vec2( dx,-dy)),
        max(SampleColor(tex, uv + vec2(-dx,  0)),
        max(SampleColor(tex, uv + vec2(  0,  0)),
        max(SampleColor(tex, uv + vec2( dx,  0)),
        max(SampleColor(tex, uv + vec2(-dx, dy)),
        max(SampleColor(tex, uv + vec2(  0, dy)),
            SampleColor(tex, uv + vec2( dx, dy)))
    )))))));

    return minima.xyz;
}

vec3 SampleLocal3x3Average(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
    float dy = 1.0f / wh.y;

    vec4 average = SampleColor(tex, uv + vec2(-dx,-dy))
        + SampleColor(tex, uv + vec2(  0,-dy))
        + SampleColor(tex, uv + vec2( dx,-dy))
        + SampleColor(tex, uv + vec2(-dx,  0))
        + SampleColor(tex, uv + vec2(  0,  0))
        + SampleColor(tex, uv + vec2( dx,  0))
        + SampleColor(tex, uv + vec2(-dx, dy))
        + SampleColor(tex, uv + vec2(  0, dy))
        + SampleColor(tex, uv + vec2( dx, dy));

    return average.xyz / 9;
}

vec3 SampleLocalCrossMinima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 minima = min(
        SampleColor(tex, uv),
        min(SampleColor(tex, uv + vec2( dx,  0)),
        min(SampleColor(tex, uv + vec2(  0, dy)),
        min(SampleColor(tex, uv + vec2(-dx,  0)),
            SampleColor(tex, uv + vec2(  0,-dy))
    ))));

    return minima.xyz;
}

vec3 SampleLocalCrossMaxima(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
	float dy = 1.0f / wh.y;

    vec4 maxima = min(
        SampleColor(tex, uv),
        max(SampleColor(tex, uv + vec2( dx,  0)),
        max(SampleColor(tex, uv + vec2(  0, dy)),
        max(SampleColor(tex, uv + vec2(-dx,  0)),
            SampleColor(tex, uv + vec2(  0,-dy))
    ))));

    return maxima.xyz;
}

vec3 SampleLocalCrossAverage(sampler2D tex, vec2 uv)
{
    ivec2 wh = textureSize(tex, 0);

    float dx = 1.0f / wh.x;
    float dy = 1.0f / wh.y;

    vec4 average = SampleColor(tex, uv)
        + SampleColor(tex, uv + vec2( dx,  0))
        + SampleColor(tex, uv + vec2(  0, dy))
        + SampleColor(tex, uv + vec2(-dx,  0))
        + SampleColor(tex, uv + vec2(  0,-dy));

    return average.xyz / 5;
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

vec3 SampleAverage(vec2 uv)
{
    vec3 average3x3 = SampleLocal3x3Average(uColorTextureSampler2D, uv);
    vec3 averageCross = SampleLocalCrossMaxima(uColorTextureSampler2D, uv);
    return mix(averageCross, average3x3, 0.5);
}

vec2 SampleVelocity(vec2 uv)
{
    vec2 velocityUV = SampleLocal3x3DepthMinimaUV(uDepthTextureSampler2D, uv);
    vec2 velocity = texture(uVelocityTextureSampler2D, velocityUV).xy;

    return velocity;
}

// https://github.com/playdeadgames/temporal
vec3 ClipAABB(vec3 aabb_min, vec3 aabb_max, vec3 p, vec3 q)
{
    const float FLT_EPS = 1e-5f;

    //note: only clips towards aabb center (but fast!)
    vec3 p_clip = 0.5 * (aabb_max + aabb_min);
    vec3 e_clip = 0.5 * (aabb_max - aabb_min) + FLT_EPS;

    vec3 v_clip = q - p_clip;
    vec3 v_unit = v_clip.xyz / e_clip;
    vec3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return p_clip + v_clip / ma_unit;
    else
        return q;
}

vec4 TemporalReprojection()
{
    vec4 reprojectedColor = vec4(0);

    const float feedback = 1.0/12.0;
    vec2 jitter = -uJitterVec2;
    vec3 fragPosPrevProj = ReverseReprojectFrag(uv, jitter);

    if (fragPosPrevProj.x > 1 || fragPosPrevProj.x < -1 || fragPosPrevProj.y > 1 || fragPosPrevProj.y < -1)
    {
        //ToDo: Gaussian Blur
        reprojectedColor = vec4(SampleColor(uColorTextureSampler2D, uv - jitter).rgb, 1);
    }
    else
    {
        vec2 prevFrameUV = vec2(
            0.5 * fragPosPrevProj.x + 0.5,
            0.5 * fragPosPrevProj.y + 0.5);


#ifdef USE_REPROJECTED_HISTORY_UV
        vec2 historyUV = prevFrameUV;
#elif  USE_VELOCITY_CORRECTED_UV
        vec2 historyUV = uv - SampleVelocity(uv - jitter);
#else
        vec2 historyUV = uv;
#endif

        if (historyUV.x >= 0 && historyUV.x <= 1 && historyUV.y >= 0 && historyUV.y <= 1)
        {
            vec3 color = SampleColor(uColorTextureSampler2D, uv - jitter).rgb;
            vec3 history = SampleColor(uHistoryTextureSampler2D, historyUV).rgb;

            vec3 minima = SampleMinima(uv - jitter);
            vec3 maxima = SampleMaxima(uv - jitter);
            vec3 average = SampleAverage(uv - jitter);

#ifdef USE_YCoCg
            vec2 chroma_extent = vec2(0.25 * 0.5 * (minima.r - maxima.r));
            vec2 chroma_center = color.gb;
            minima.yz = chroma_center - chroma_extent;
            maxima.yz = chroma_center + chroma_extent;
            average.yz = chroma_center;
#endif

#ifdef USE_CLIPPING
            history = ClipAABB(minima, maxima, clamp(average, minima, maxima), history);
#elif  USE_CLAMPING
            history = clamp(texture(uHistoryTextureSampler2D, historyUV).rgb,
                SampleMinima(uv - jitter), SampleMaxima(uv - jitter));
#endif

            reprojectedColor = vec4(mix(history, color, feedback), 1);
        }
        else
        {
            //ToDo: Gaussian Blur
            reprojectedColor = vec4(SampleColor(uColorTextureSampler2D, uv).rgb, 1);
        }
    }

    return reprojectedColor;
}

void main()
{
    if (uFrameCountUint < 2)
    {
        outColor = ResolveColor(vec4(SampleColor(uColorTextureSampler2D, uv).rgb, 1));
    }
    else
    {
        vec4 reprojectedColor = TemporalReprojection();
        outColor = ResolveColor(reprojectedColor);
    }
}

