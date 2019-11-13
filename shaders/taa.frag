#version 460

layout (location = 10) uniform mat4 uViewMat;
layout (location = 11) uniform mat4 uProjMat;
layout (location = 14) uniform mat4 uProjUnjitMat;
layout (location = 12) uniform mat4 uPrevViewMat;
layout (location = 13) uniform mat4 uPrevProjMat;
layout (location = 15) uniform mat4 uPrevProjUnjitMat;
layout (location = 16) uniform uint uFrameCountUint;
layout (location = 17) uniform vec2 uJitterVec2;
layout (location = 18) uniform uint uTaaEnabledUint;
layout (location = 19) uniform uint uTaaJitterEnabledUint;

layout (location = 20, binding = 0) uniform sampler2D uColorTextureSampler2D;
layout (location = 21, binding = 1) uniform sampler2D uDepthTextureSampler2D;
layout (location = 22, binding = 2) uniform sampler2D uHistoryTextureSampler2D;
layout (location = 23, binding = 3) uniform sampler2D uVelocityTextureSampler2D;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outDebugColor;

#define USE_AABB_CLIPPING
#define USE_VELOCITY_CORRECTED_UV

// https://software.intel.com/en-us/node/503873
vec3 RGB2YCoCg(vec3 rgb)
{
    float co = rgb.r - rgb.b;
    float t = rgb.b + co / 2.0;
    float cg = rgb.g - t;
    float y = t + cg / 2.0;
    return vec3(y, co, cg);
}

// https://software.intel.com/en-us/node/503873
vec3 YCoCg2RGB(vec3 ycocg)
{
    float t = ycocg.r - ycocg.b / 2.0;
    float g = ycocg.b + t;
    float b = t - ycocg.g / 2.0;
    float r = ycocg.g + b;
    return vec3(r, g, b);
}

vec4 SampleColorTexture(sampler2D tex, vec2 uv)
{
    vec4 color = texture(tex, uv);
    return vec4(RGB2YCoCg(color.rgb), color.a);
}

vec4 ResolveSampleColor(vec4 color)
{
    return vec4(YCoCg2RGB(color.xyz), color.a);
}

vec3 WorldPosFromDepth(float depth, vec2 TexCoord)
{
    vec4 clipSpacePosition = vec4(TexCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = bool(uTaaJitterEnabledUint)
        ? inverse(uProjMat) * clipSpacePosition
        : inverse(uProjUnjitMat) * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = inverse(uViewMat) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec3 ReverseReprojectFrag(vec2 uv, vec2 jitter)
{
    vec3 fragPosWorld = WorldPosFromDepth(texture(uDepthTextureSampler2D, uv - jitter).r, uv);

    vec4 fragPosPrevProj = bool(uTaaJitterEnabledUint)
        ? uPrevProjMat * uPrevViewMat * vec4(fragPosWorld, 1)
        : uPrevProjUnjitMat * uPrevViewMat * vec4(fragPosWorld, 1);

    return fragPosPrevProj.xyz / fragPosPrevProj.w;
}

vec2 SampleLocal3x3DepthMinimaUV(sampler2D tex, vec2 uv)
{
    ivec2 textureResolution = textureSize(tex, 0);

    float dx = 1.0f / textureResolution.x;
	float dy = 1.0f / textureResolution.y;

    vec2 minimaUV = uv;
    float minimaDepth = texture(tex, minimaUV).r;
    for (float y = -2; y < 3; y+=1)
    {
        for (float x = -2; x < 3; x+=1)
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
    ivec2 textureResolution = textureSize(tex, 0);

    float dx = 1.0f / textureResolution.x;
	float dy = 1.0f / textureResolution.y;

    vec4 minima =
        min(SampleColorTexture(tex, uv + vec2(-dx,-dy)),
        min(SampleColorTexture(tex, uv + vec2(  0,-dy)),
        min(SampleColorTexture(tex, uv + vec2( dx,-dy)),
        min(SampleColorTexture(tex, uv + vec2(-dx,  0)),
        min(SampleColorTexture(tex, uv + vec2( dx,  0)),
        min(SampleColorTexture(tex, uv + vec2(-dx, dy)),
        min(SampleColorTexture(tex, uv + vec2(  0, dy)),
            SampleColorTexture(tex, uv + vec2( dx, dy)))
    ))))));

    return minima.xyz;
}

vec3 SampleLocal3x3Maxima(sampler2D tex, vec2 uv)
{
    ivec2 textureResolution = textureSize(tex, 0);

    float dx = 1.0f / textureResolution.x;
	float dy = 1.0f / textureResolution.y;

    vec4 minima =
        max(SampleColorTexture(tex, uv + vec2(-dx,-dy)),
        max(SampleColorTexture(tex, uv + vec2(  0,-dy)),
        max(SampleColorTexture(tex, uv + vec2( dx,-dy)),
        max(SampleColorTexture(tex, uv + vec2(-dx,  0)),
        max(SampleColorTexture(tex, uv + vec2(  0,  0)),
        max(SampleColorTexture(tex, uv + vec2( dx,  0)),
        max(SampleColorTexture(tex, uv + vec2(-dx, dy)),
        max(SampleColorTexture(tex, uv + vec2(  0, dy)),
            SampleColorTexture(tex, uv + vec2( dx, dy)))
    )))))));

    return minima.xyz;
}

vec3 SampleLocal3x3Average(sampler2D tex, vec2 uv)
{
    ivec2 textureResolution = textureSize(tex, 0);

    float dx = 1.0f / textureResolution.x;
    float dy = 1.0f / textureResolution.y;

    vec4 average = SampleColorTexture(tex, uv + vec2(-dx,-dy))
        + SampleColorTexture(tex, uv + vec2(  0,-dy))
        + SampleColorTexture(tex, uv + vec2( dx,-dy))
        + SampleColorTexture(tex, uv + vec2(-dx,  0))
        + SampleColorTexture(tex, uv + vec2(  0,  0))
        + SampleColorTexture(tex, uv + vec2( dx,  0))
        + SampleColorTexture(tex, uv + vec2(-dx, dy))
        + SampleColorTexture(tex, uv + vec2(  0, dy))
        + SampleColorTexture(tex, uv + vec2( dx, dy));

    return average.xyz / 9;
}

vec3 SampleLocalCrossMinima(sampler2D tex, vec2 uv)
{
    ivec2 textureResolution = textureSize(tex, 0);

    float dx = 1.0f / textureResolution.x;
	float dy = 1.0f / textureResolution.y;

    vec4 minima = min(
        SampleColorTexture(tex, uv),
        min(SampleColorTexture(tex, uv + vec2( dx,  0)),
        min(SampleColorTexture(tex, uv + vec2(  0, dy)),
        min(SampleColorTexture(tex, uv + vec2(-dx,  0)),
            SampleColorTexture(tex, uv + vec2(  0,-dy))
    ))));

    return minima.xyz;
}

vec3 SampleLocalCrossMaxima(sampler2D tex, vec2 uv)
{
    ivec2 textureResolution = textureSize(tex, 0);

    float dx = 1.0f / textureResolution.x;
	float dy = 1.0f / textureResolution.y;

    vec4 maxima = min(
        SampleColorTexture(tex, uv),
        max(SampleColorTexture(tex, uv + vec2( dx,  0)),
        max(SampleColorTexture(tex, uv + vec2(  0, dy)),
        max(SampleColorTexture(tex, uv + vec2(-dx,  0)),
            SampleColorTexture(tex, uv + vec2(  0,-dy))
    ))));

    return maxima.xyz;
}

vec3 SampleLocalCrossAverage(sampler2D tex, vec2 uv)
{
    ivec2 textureResolution = textureSize(tex, 0);

    float dx = 1.0f / textureResolution.x;
    float dy = 1.0f / textureResolution.y;

    vec4 average = SampleColorTexture(tex, uv)
        + SampleColorTexture(tex, uv + vec2( dx,  0))
        + SampleColorTexture(tex, uv + vec2(  0, dy))
        + SampleColorTexture(tex, uv + vec2(-dx,  0))
        + SampleColorTexture(tex, uv + vec2(  0,-dy));

    return average.xyz / 5;
}

vec3 SampleMinima(vec2 uv, bool isVelocitySubpixel)
{
    vec3 minimaCross = SampleLocalCrossMinima(uColorTextureSampler2D, uv);
    vec3 minima3x3 = SampleLocal3x3Minima(uColorTextureSampler2D, uv);
    if (isVelocitySubpixel)
    {
        return mix(minimaCross, minima3x3, 0.5);
    }
    return minimaCross;
}

vec3 SampleMaxima(vec2 uv, bool isVelocitySubpixel)
{
    vec3 maximaCross = SampleLocalCrossMaxima(uColorTextureSampler2D, uv);
    vec3 maxima3x3 = SampleLocal3x3Maxima(uColorTextureSampler2D, uv);
    if (isVelocitySubpixel)
    {
        return mix(maximaCross, maxima3x3, 0.5);
    }
    return maximaCross;
}

vec3 SampleAverage(vec2 uv, bool isVelocitySubpixel)
{
    vec3 averageCross = SampleLocalCrossMaxima(uColorTextureSampler2D, uv);
    vec3 average3x3 = SampleLocal3x3Average(uColorTextureSampler2D, uv);
    if (isVelocitySubpixel)
    {
        return mix(averageCross, average3x3, 0.5);
    }
    return averageCross;
}

vec2 SampleDilateVelocity(vec2 uv)
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

// https://www.shadertoy.com/view/MscSD7
vec3 ClipToAABB(in vec3 centre, in vec3 halfSize, in vec3 cNew, in vec3 cOld)
{
    if (all(lessThanEqual(abs(cOld - centre), halfSize))) {
        return cOld;
    }

    vec3 dir = (cNew - cOld);
    vec3 near = centre - sign(dir) * halfSize;
    vec3 tAll = (near - cOld) / dir;
    float t = 1e20;
    for (int i = 0; i < 3; i++) {
        if (tAll[i] >= 0.0 && tAll[i] < t) {
            t = tAll[i];
        }
    }

    if (t >= 1e20) {
		return cOld;
    }
    return cOld + dir * t;
}

vec4 TemporalReprojection()
{
    vec4 reprojectedColor = vec4(0);

    float feedback = 0.05f;
    //vec2 jitter = bool(uTaaJitterEnabledUint) ? -uJitterVec2 : vec2(0);
    vec2 jitter = vec2(0);
    vec2 uvUnjit = uv - jitter;
    vec3 fragPosPrevProj = ReverseReprojectFrag(uv, jitter);

    if (fragPosPrevProj.x > 1 || fragPosPrevProj.x < -1 || fragPosPrevProj.y > 1 || fragPosPrevProj.y < -1)
    {
        //ToDo: Gaussian Blur
        reprojectedColor = vec4(SampleColorTexture(uColorTextureSampler2D, uvUnjit).rgb, 1);
    }
    else
    {
        ivec2 textureResolution = textureSize(uDepthTextureSampler2D, 0);
        vec2 dxdy =  1.f / textureResolution;
        vec2 velocity = SampleDilateVelocity(uv);
        const bool isVelocitySubpixel = abs(velocity.x) < dxdy.x / 2 && abs(velocity.y) < dxdy.y / 2;

#ifdef  USE_VELOCITY_CORRECTED_UV
        vec2 historyUV = uv - velocity;
#else
        vec2 historyUV = uv;
#endif

        if (historyUV.x >= 0 && historyUV.x <= 1 && historyUV.y >= 0 && historyUV.y <= 1)
        {
            vec3 color = SampleColorTexture(uColorTextureSampler2D, uvUnjit).rgb;
            vec4 history = SampleColorTexture(uHistoryTextureSampler2D, historyUV);

            vec3 minima = SampleMinima(uvUnjit, isVelocitySubpixel);
            vec3 maxima = SampleMaxima(uvUnjit, isVelocitySubpixel);
            vec3 average = SampleAverage(uvUnjit, isVelocitySubpixel);

#ifdef USE_AABB_CLIPPING
            vec2 chroma_extent = vec2(0.25 * 0.5 * (minima.r - maxima.r));
            vec2 chroma_center = color.gb;
            minima.yz = chroma_center - chroma_extent;
            maxima.yz = chroma_center + chroma_extent;
            average.yz = chroma_center;

            //Check if local velocity is subpixel
            if (isVelocitySubpixel)
            {
                feedback = 0.05f;
                outDebugColor = mix(vec4(0, 0.5, 0, 1), ResolveSampleColor(vec4(color, 1)), 0.5);
            }
            else
            {
                vec2 luma = vec2(color.x, history.x);
                float weight = 1. - abs(luma.x - luma.y) / max(luma.x, max(luma.y, .2));
                feedback = mix(0.5f, 0.05f, weight * weight);
                outDebugColor = mix(vec4(0.5, 0, 0, 1), ResolveSampleColor(vec4(color, 1)), 0.5);
            }
            feedback = 0.1f;

            if (false)
            {
                // https://www.shadertoy.com/view/MscSD7
                vec2 offsets[4];
                offsets[0] = vec2(-1.0,  0.0);
                offsets[1] = vec2( 1.0,  0.0);
                offsets[2] = vec2( 0.0, -1.0);
                offsets[3] = vec2( 0.0,  1.0);

                vec3 mean = color.rgb;
                vec3 stddev = mean * mean;
                for (int i = 0; i < 4; i++) {
                    vec3 c = SampleColorTexture(
                        uColorTextureSampler2D,
                        uv + offsets[i] / textureResolution.xy).rgb;
                    mean += c;
                    stddev += c * c;
                }
                mean /= 5.0;
                stddev = sqrt(stddev / 5.0 - mean * mean);

                //history = vec4(ClipToAABB(mean, stddev, color.rgb, history.rgb), history.a);
                history = vec4(ClipAABB(mean - stddev, mean + stddev, color.rgb, history.rgb), history.a);
            }
            else
            {
                vec4 new_history = vec4(
                    ClipAABB(minima, maxima, clamp(average, minima, maxima), history.rgb), history.a);
                new_history.a = float(new_history != history);
                if (new_history.a != 0)
                {
                    new_history.a = mix(new_history.a, history.a, feedback);
                }

                history = vec4(mix(history.rgb, new_history.rgb, new_history.a), new_history.a);
            }
#else // AABB clamping
            history = vec4(
                clamp(
                    SampleColorTexture(uHistoryTextureSampler2D, historyUV).rgb,
                    SampleMinima(uvUnjit, isVelocitySubpixel),
                    SampleMaxima(uvUnjit, isVelocitySubpixel)
                ), history.a);
#endif

            reprojectedColor = vec4(mix(history.rgb, color, feedback), 1);
        }
        else
        {
            //ToDo: Gaussian Blur
            reprojectedColor = vec4(SampleColorTexture(uColorTextureSampler2D, uv).rgb, 1);
        }
    }

    return reprojectedColor;
}

void main()
{
    if (bool(uTaaEnabledUint))
    {
        outColor = uFrameCountUint < 2
            ? ResolveSampleColor(vec4(SampleColorTexture(uColorTextureSampler2D, uv).rgb, 1))
            : ResolveSampleColor(TemporalReprojection());
    }
    else
    {
        outColor = ResolveSampleColor(vec4(SampleColorTexture(uColorTextureSampler2D, uv).rgb, 1));
    }
}

