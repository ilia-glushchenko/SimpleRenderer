#version 460

layout (location = 13) uniform uint uRenderModeUvUint;

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

float nearest_neighbour_checkerboard_color(vec2 inUv)
{
    return mod(floor(inUv.x) + floor(inUv.y), 2) < 1 ? 1 : 0;
}

float filter_width(vec2 v)
{
    vec2 fw = max(abs(dFdxFine(v)), abs(dFdyFine(v)));
    return max(fw.x, fw.y);
}

vec2 bump_int(vec2 x)
{
    return vec2(
        floor(x/2) + 2.f * max(x/2 - floor(x/2) - .5f, 0.f)
    );
}

float mipmap_checkerboard_color(vec2 inUv)
{
    float width = filter_width(inUv);

    vec2 p0 = inUv - .5 * width;
    vec2 p1 = inUv + .5 * width;

    vec2 i = (bump_int(p1) - bump_int(p0)) / width;

    return i.x * i.y + (1 - i.x) * (1 - i.y);
}

float anisatropic_checkerboard_color(vec2 inUv)
{
    float samples_count = 32.f;
    float width = filter_width(inUv) / samples_count;
    float w = width;
    float color = 0;

    for (int j = 0; j < samples_count; ++j)
    {
        vec2 p0 = inUv - .5 * w;
        vec2 p1 = inUv + .5 * w;

        vec2 i = (bump_int(p1) - bump_int(p0)) / w;

        color += (i.x * i.y + (1 - i.x) * (1 - i.y));
        w += width;
    }

    return color / samples_count;
}

void main()
{
    outColor = vec4(0);

    if (uRenderModeUvUint == 0) // UV
    {
        float color = nearest_neighbour_checkerboard_color(inUv * 20);
        outColor = vec4(color, color, color, 1);
    }
    else if (uRenderModeUvUint == 1) // UV mipmap
    {
        float color = mipmap_checkerboard_color(inUv * 20);
        outColor = vec4(color, color, color, 1);
    }
    else if (uRenderModeUvUint == 2) // UV anisatropic
    {
        float color = anisatropic_checkerboard_color(inUv * 20);
        outColor = vec4(color, color, color, 1);
    }
}