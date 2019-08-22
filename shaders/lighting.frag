#version 460

layout (location = 10) uniform vec3 uColor;
layout (location = 11) uniform mat4 uModelMat;

layout (location = 17) uniform uint uRenderModeUint;
layout (location = 18) uniform uint uShadowMappingEnabledUint;
layout (location = 19) uniform uint uBumpMapAvailableUint;
layout (location = 20) uniform uint uMetallicMapAvailableUint;
layout (location = 21) uniform uint uRoughnessMapAvailableUint;
layout (location = 22) uniform uint uBumpMappingEnabledUint;
layout (location = 23) uniform float uBumpMapScaleFactorFloat;
layout (location = 24) uniform vec3 uCameraPos;
layout (location = 25) uniform vec3 uPointLightPos;
layout (location = 26) uniform uint uDebugRenderModeAvailableUint;
layout (location = 27) uniform vec3 uPointLightPosVec3Array[5];

layout (binding = 0) uniform sampler2D uShadowMapSampler2D;
layout (binding = 1) uniform sampler2D uAlbedoMapSampler2D;
layout (binding = 2) uniform sampler2D uNormalMapSampler2D;
layout (binding = 3) uniform sampler2D uBumpMapSampler2D;
layout (binding = 4) uniform sampler2D uMetallicSampler2D;
layout (binding = 5) uniform sampler2D uRoughnessSampler2D;

layout (location = 0) in vec4 positionWorld;
layout (location = 1) in vec4 positionView;
layout (location = 2) in vec3 normalWorld;
layout (location = 3) in vec3 normalView;
layout (location = 4) in vec3 directionalLightDir;
layout (location = 5) in vec4 positionShadowMapMvp;
layout (location = 6) in vec2 inUv;

layout (location = 0) out vec4 outColor;

const float MipBias = 0;
const float PI = 3.1415926535897932384626433832795;
const vec3 SUN_LIGHT_COLOR = vec3(252.0 / 255.0, 212/ 255.0, 64 / 255.0); // Sun
const vec3 POINT_LIGHT_COLOR = vec3(255.0 / 255.0, 209/ 255.0, 163 / 255.0); // 4000k
const float SilkIOR = 1.5605f;
const float MarbleIOR = 1.486f;
const float AirIOR = 1.00029f;

vec3 AnisatropicTextureSample(sampler2D samp, vec2 sampleUV)
{
    vec2 dx = dFdxFine(sampleUV.xy) * 0.25; // horizontal offset
    vec2 dy = dFdyFine(sampleUV.xy) * 0.25; // vertical offset
    // supersampled 2x2 ordered grid
    vec3 col = vec3(0);
    col += texture(samp, sampleUV.xy + dx + dy, MipBias).rgb;
    col += texture(samp, sampleUV.xy - dx + dy, MipBias).rgb;
    col += texture(samp, sampleUV.xy + dx - dy, MipBias).rgb;
    col += texture(samp, sampleUV.xy - dx - dy, MipBias).rgb;
    col *= 0.25;

    //return col;

    return texture(samp, sampleUV, MipBias).rgb;
}

mat3 CalculateTBNMatrix( vec3 N, vec3 p, vec2 pUV )
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( pUV );
    vec2 duv2 = dFdy( pUV );

    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

float Chi(vec3 n, vec3 h)
{
    return dot(n, h) > 0 ? 1 : 0;
}

float BeckmannNDF(vec3 n, vec3 h, float ro)
{
    float NoHSq = pow(dot(n, h), 2);
    float RoSq = ro * ro;
    return Chi(n, h) / (PI * RoSq * pow(NoHSq, 2)) * exp((NoHSq - 1) / (RoSq * NoHSq));
}

float GinnekenLambdaFunction(vec3 v, vec3 l)
{
    float fi = acos(dot(v, l));
    return (4.41 * fi) / (4.41 * fi + 1);
}

float BeckmannLambdaFunction(vec3 n, vec3 s, float ro)
{
    float NoS = dot(n, s);
    float a =  NoS / (ro * sqrt(1 - NoS*NoS));

    return 1;
    return a < 1.6 ? (1 - 1.259*a + 0.396*a*a) / (3.535*a + 2.181*a*a) : 0;
}

float SmithDirectionHeightCorrelatedMaskingShadowingFunction(
    float lambdaL, float lambdaV, float lambdaVL
)
{
    return 1 / (1 + max(lambdaV, lambdaL) + lambdaVL * min(lambdaV, lambdaL));
}

float SmithHeightCorrelatedMaskingShadowingFunction(
    float lambdaL, float lambdaV
)
{
    return 1 / (1 + lambdaV + lambdaL);
}

float FresnelSchlickF0(float iorMedium, float iorInterface)
{
    return pow((iorInterface - iorMedium)/(iorInterface + iorMedium), 2);
}

float FresnelSchlick(float F0, vec3 n, vec3 l)
{
    return F0 + (1 - F0) * pow((1 - max(0, dot(n, l))), 5);
}

float FresnelSchlick(vec3 v, vec3 l)
{
    vec3 h = (l + v) / length(l + v);
    return FresnelSchlick(FresnelSchlickF0(AirIOR, MarbleIOR), h, l);
}

float CookTorance(vec3 v, vec3 n, vec3 l, float ro)
{
    vec3 h = (l + v) / length(l + v);

    float D = BeckmannNDF(n, h, ro);
    float G2 = SmithDirectionHeightCorrelatedMaskingShadowingFunction(
        BeckmannLambdaFunction(n, l, ro),
        BeckmannLambdaFunction(n, v, ro),
        GinnekenLambdaFunction(v, l)
    );
    float F = FresnelSchlick(FresnelSchlickF0(AirIOR, MarbleIOR), h, l);

    return (F * G2 * D) / (4 * abs(dot(n, l)) * abs(dot(n, v)));
}

vec3 ShirleyFresnelSubSurfaceAlbedo(float F0, vec3 ssAlbedo, vec3 v, vec3 n, vec3 l)
{
    return 21.f / (20.f * PI) * (1 - F0) * ssAlbedo
        * (1 - pow(1 - max(dot(n, l), 0), 5))
        * (1 - pow(1 - max(dot(n, v), 0), 5));
}

float LightFalloffWindowingFunction(float r0, float rMin, float rMax, float r)
{
    return pow(r0 / max(r, rMin), 2) * pow(max(1 - pow(r / rMax, 4), 0), 2);
}

vec3 CalculateRadiance(float depth, vec2 uv, vec3 n, vec3 shadowPosMVP, mat3 TBN)
{
    vec3 radiance = vec3(0);

    uint i = 1;
    //for (uint i = 0; i < 13; i+=5)
    {
        vec3 view = -normalize(positionWorld.xyz / positionWorld.w - uCameraPos);

        if (bool(uBumpMapAvailableUint) && bool(uBumpMappingEnabledUint)) {
            float h = AnisatropicTextureSample(uBumpMapSampler2D, uv).r;
            uv = uv + h * (TBN * view.xyz).xy * uBumpMapScaleFactorFloat;
        }

        vec3 ssAlbedo = AnisatropicTextureSample(uAlbedoMapSampler2D, uv).rgb;
        vec3 normal = normalize((AnisatropicTextureSample(uNormalMapSampler2D, uv).xyz * 2) - 1);
        normal = normalize(n + TBN * normal);
        normal = n;
        float ro = bool(uRoughnessMapAvailableUint) ? AnisatropicTextureSample(uRoughnessSampler2D, uv).r : 1;

        float fSpecDL = 0;
        if (bool(uShadowMappingEnabledUint) && abs(shadowPosMVP.z) < depth - 0.45f) {
            fSpecDL = CookTorance(view, normal, directionalLightDir, ro);
        }

        vec3 pointLightDir = -normalize(positionWorld.xyz / positionWorld.w - uPointLightPosVec3Array[i]);
        vec3 diff = ShirleyFresnelSubSurfaceAlbedo(FresnelSchlickF0(AirIOR, MarbleIOR), ssAlbedo, view, normal, pointLightDir);
        float fSpecPL = CookTorance(view, normal, pointLightDir, ro);
        float d = distance(uPointLightPosVec3Array[i], positionWorld.xyz / positionWorld.w);
        vec3 spec = LightFalloffWindowingFunction(1000, 1, 10000, d) *
             PI * fSpecPL * POINT_LIGHT_COLOR * max(dot(n, pointLightDir), 0);

        radiance += diff + spec;
    }

    return radiance;
}

void main()
{
    outColor = vec4(1, 0, 0, 1);
    vec3 n = normalize(normalWorld);
    vec3 l = normalize(directionalLightDir);

    vec3 shadowPosMVP = positionShadowMapMvp.xyz / positionShadowMapMvp.w;
    float depth = AnisatropicTextureSample(uShadowMapSampler2D, (shadowPosMVP.xy+1)*0.5f).x;

    mat3 TBN = CalculateTBNMatrix(n, positionWorld.xyz / positionWorld.w, inUv);
    vec2 uv = inUv;

    if (uRenderModeUint == 0) // Full
    {
        if (bool(uDebugRenderModeAvailableUint)){
            outColor = vec4(uColor, 1);
        }
        else{
            vec3 radiance = CalculateRadiance(depth, uv, n, shadowPosMVP, TBN);
            outColor = vec4(radiance, 1);
        }
    }
    else if (uRenderModeUint == 1) // Normal
    {
        outColor = vec4((n + 1) * 0.5f, 1);
    }
    else if (uRenderModeUint == 2) // NormalMap
    {
        if (bool(uBumpMapAvailableUint) && bool(uBumpMappingEnabledUint))
        {
            vec3 view = -normalize(positionWorld.xyz / positionWorld.w - uCameraPos);
            float h = AnisatropicTextureSample(uBumpMapSampler2D, uv).r;
            uv = uv + h * (TBN * view.xyz).xy * uBumpMapScaleFactorFloat;

        }
        vec3 normal = normalize((texture(uNormalMapSampler2D, uv, MipBias).xyz * 2) - 1);
        normal = normalize(n + TBN * normal);

        outColor = vec4((n + normal + 1) * 0.5f, 1);
    }
    else if(uRenderModeUint == 3) // BumpMap
    {
        outColor = vec4(n + vec3(1, 0, 0), 1);
        if (bool(uBumpMapAvailableUint))
        {
            float bump = texture(uBumpMapSampler2D, uv, MipBias).r;
            outColor = vec4(bump, bump, bump, 1);
        }
    }
    else if (uRenderModeUint == 4) // Depth
    {
        outColor = vec4(vec3(pow(gl_FragCoord.z, 4096)), 1);
    }
    else if (uRenderModeUint == 5) // ShadowMap
    {
        outColor = vec4(vec3(pow(depth, 4096)), 1);
    }
    else if (uRenderModeUint == 6) // Metallic
    {
        outColor = bool(uMetallicMapAvailableUint)
            ? vec4(texture(uMetallicSampler2D, uv, MipBias).rrr, 1)
            : vec4(n + vec3(1, 0, 0), 1);
    }
    else if (uRenderModeUint == 7) // Roughness
    {
        outColor = bool(uRoughnessMapAvailableUint)
            ? vec4(texture(uRoughnessSampler2D, uv, MipBias).rrr, 1)
            : vec4(n + vec3(1, 0, 0), 1);
    }
}
