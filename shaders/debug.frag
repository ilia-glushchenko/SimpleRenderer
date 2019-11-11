#version 460

layout (location = 10) uniform uint uTaaEnabledUint;

layout (location = 20, binding = 0) uniform sampler2D uLightingColorTextureSampler2D;
layout (location = 21, binding = 1) uniform sampler2D uLightingDepthTextureSampler2D;
layout (location = 24, binding = 2) uniform sampler2D uVelocityTextureSampler2D;
layout (location = 25, binding = 3) uniform sampler2D uToneMappingTextureSampler2D;
layout (location = 22, binding = 4) uniform sampler2D uTaaHistoryTextureSampler2D;
layout (location = 23, binding = 5) uniform sampler2D uTaaDrawTextureSampler2D;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 outColor;

// Converts a single linear channel to srgb
float Linear2sRGB(float channel)
{
    if(channel <= 0.0031308)
        return 12.92 * channel;
    else
        return (1.0 + 0.055) * pow(channel, 1.0/2.4) - 0.055;
}

vec3 Linear2sRGB(vec3 color)
{
    return vec3(
        Linear2sRGB(color.r),
        Linear2sRGB(color.g),
        Linear2sRGB(color.b)
    );
}

bool IsNan(vec4 color)
{
    return isnan(color.x) || isnan(color.y) || isnan(color.z) || isnan(color.w);
}

// #define NAN
// #define TONE_MAPPING
// #define COLOR_DIST
//#define VELOCITY

void main()
{
    vec3 lightingColor = texture(uLightingColorTextureSampler2D, uv).rgb;
    vec3 toneMappedColor = texture(uToneMappingTextureSampler2D, uv).rgb;
    vec3 taaColor = texture(uTaaDrawTextureSampler2D, uv).rgb;

    outColor = vec4(toneMappedColor, 1);

#ifdef NAN
    vec4 color = texture(uLightingColorTextureSampler2D, uv);
    outColor = IsNan(color) ? vec4(1, 0, 0, 1) : color * 0.1;
#endif

#ifdef TONE_MAPPING
    outColor = vec4(texture(uLightingColorTextureSampler2D, uv).rgb
        -texture(uToneMappingTextureSampler2D, uv).rgb, 1);
#endif

#ifdef COLOR_DIST
    outColor = distance(toneMappedColor, taaColor) > 0.1f
       ? vec4(1, 0, 0, 1) : vec4(0, 1, 0.5, 1);
#endif

#ifdef VELOCITY
    outColor = vec4(abs(texture(uVelocityTextureSampler2D, uv).xy) * 100, 0, 1);
#endif

    //outColor = vec4((toneMappedColor - taaColor) * 1000f, 1);

    //outColor = vec4(mix(
    //    texture(uLightingColorTextureSampler2D, uv).rgb,
    //    texture(uVelocityTextureSampler2D, uv).rgb, 0.5), 1);
    //outColor = texture(uTaaHistoryTextureSampler2D, uv);
    //outColor = vec4(abs(texture(uTaaDrawTextureSampler2D, uv).rgb - texture(uLightingColorTextureSampler2D, uv).rgb), 1);
    //outColor = texture(uTaaDrawTextureSampler2D, uv);

    // outColor = vec4(vec3(length(texture(uVelocityTextureSampler2D, uv).rgb)) * 1000, 1);
    // outColor = vec4(vec3(length(abs(texture(uVelocityTextureSampler2D, uv).rgb))), 1);
    //outColor = vec4(
    //    abs(texture(uToneMappingTextureSampler2D, uv).rgb - texture(uTaaDrawTextureSampler2D, uv).rgb),
    //    1);
    //outColor = vec4(vec3(pow(texture(uLightingDepthTextureSampler2D, uv).r, 1024)), 1);
    //outColor = vec4(Linear2sRGB(outColor.xyz), 1);
}
