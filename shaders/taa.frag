#version 440

layout(binding = 0) uniform sampler2D uColorTextureSampler2D;
layout(binding = 1) uniform sampler2D uDepthTextureSampler2D;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 outColor;

vec3 depth()
{
    float v = pow(texture(uDepthTextureSampler2D, uv).r , 4096);
	return vec3(v,v,v);
}

void main()
{
    vec3 color = texture(uColorTextureSampler2D, uv).xyz;
    outColor = vec4(color, 1);    
    //outColor = vec4(depth(), 1);
}
