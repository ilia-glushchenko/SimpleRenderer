#version 460

layout (location = 10, binding = 0) uniform sampler2D uLinearSpaceSceneReferredTextureSampler2D;
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

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

float sRGB2Linear(float channel)
{
    if (channel <= 0.04045)
        return channel / 12.92;
    else
        return pow((channel + 0.055) / (1.0 + 0.055), 2.4);
}

vec3 sRGB2Linear(vec3 color)
{
    return vec3(
        sRGB2Linear(color.r),
        sRGB2Linear(color.g),
        sRGB2Linear(color.b)
    );
}

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = color * ACESInputMat;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = color * ACESOutputMat;

    // Clamp to [0, 1]
    color = clamp(color, 0, 1);

    return color;
}

void main()
{
    //ToDo: Tone Mapping is not physically correct before TAA, can do better
    //  see https://de45xmedrsdbp.cloudfront.net/Resources/files/TemporalAA_small-59732822.pdf
    outColor = vec4(ACESFitted(texture(uLinearSpaceSceneReferredTextureSampler2D, uv).rgb), 1);
}
