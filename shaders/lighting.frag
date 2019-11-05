#version 460

layout (location = 10) uniform vec3 uColor;
layout (location = 11) uniform mat4 uModelMat;

layout (location = 20) uniform uint uRenderModeUint;
layout (location = 21) uniform uint uPointLightEnabledUint;
layout (location = 22) uniform uint uDirectLightEnabledUint;
layout (location = 23) uniform uint uTaaJitterEnabledUint;
layout (location = 24) uniform uint uShadowMappingEnabledUint;
layout (location = 26) uniform uint uBumpMapAvailableUint;
layout (location = 27) uniform uint uMetallicMapAvailableUint;
layout (location = 28) uniform uint uRoughnessMapAvailableUint;
layout (location = 29) uniform uint uBumpMappingEnabledUint;
layout (location = 30) uniform float uBumpMapScaleFactorFloat;
layout (location = 31) uniform vec3 uCameraPos;
layout (location = 32) uniform uint uDebugRenderModeAvailableUint;
layout (location = 33) uniform uint uBrdfUint;
layout (location = 34) uniform vec3 uPointLightPosVec3Array[5];

layout (binding = 0, location = 50) uniform sampler2D uShadowMapSampler2D;
layout (binding = 1, location = 51) uniform sampler2D uAlbedoMapSampler2D;
layout (binding = 2, location = 52) uniform sampler2D uNormalMapSampler2D;
layout (binding = 3, location = 53) uniform sampler2D uBumpMapSampler2D;
layout (binding = 4, location = 54) uniform sampler2D uMetallicSampler2D;
layout (binding = 5, location = 55) uniform sampler2D uRoughnessSampler2D;

layout (location = 0) in vec4 positionWorld;
layout (location = 1) in vec4 positionView;
layout (location = 2) in vec3 normalWorld;
layout (location = 3) in vec3 normalView;
layout (location = 4) in vec3 directionalLightDir;
layout (location = 5) in vec4 positionShadowMapMvp;
layout (location = 6) in vec2 inUv;

layout (location = 0) out vec4 outColor;

const float MipBias = -1.0;
const float PI = 3.1415926535897932384626433832795;
const vec3 SUN_LIGHT_COLOR = vec3(252.0 / 255.0, 212/ 255.0, 64 / 255.0); // Sun
const vec3 POINT_LIGHT_COLOR = vec3(255.0 / 255.0, 209/ 255.0, 163 / 255.0); // 4000k
const float SilkIOR = 1.5605f;
const float MarbleIOR = 1.486f;
const float LinenIOR = 5.14593f;
const float AirIOR = 1.00029f;
const float SMALL_EPS = 1e-5f;
const float BIG_EPS = 0.1f;

vec3 AnisatropicTextureSample(sampler2D samp, vec2 sampleUV)
{
    // per pixel partial derivatives
    vec2 dx = dFdxFine(sampleUV.xy);
    vec2 dy = dFdyFine(sampleUV.xy);

    // rotated grid uv offsets
    vec2 uvOffsets = vec2(0.125, 0.375);
    vec4 offsetUV = vec4(0.0, 0.0, 0.0, MipBias);

    // supersampled using 2x2 rotated grid
    vec3 color = vec3(0);
    offsetUV.xy = sampleUV.xy + uvOffsets.x * dx + uvOffsets.y * dy;
    color += texture(samp, offsetUV.xy, MipBias).rgb;
    offsetUV.xy = sampleUV.xy - uvOffsets.x * dx - uvOffsets.y * dy;
    color += texture(samp, offsetUV.xy, MipBias).rgb;
    offsetUV.xy = sampleUV.xy + uvOffsets.y * dx - uvOffsets.x * dy;
    color += texture(samp, offsetUV.xy, MipBias).rgb;
    offsetUV.xy = sampleUV.xy - uvOffsets.y * dx + uvOffsets.x * dy;
    color += texture(samp, offsetUV.xy, MipBias).rgb;
    color *= 0.25;

    return color;
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
    float RoSq = ro * ro + SMALL_EPS;
    return Chi(n, h) / (PI * RoSq * pow(NoHSq, 2)) * exp((NoHSq - 1) / (RoSq * NoHSq));
}

float GinnekenLambdaFunction(vec3 v, vec3 l)
{
    float VoL = clamp(dot(v, l), -1, 1);
    float fi = acos(VoL);
    return (4.41 * (fi + SMALL_EPS)) / (4.41 * fi + 1);
}

float BeckmannLambdaFunction(vec3 n, vec3 s, float ro)
{
    float NoS = dot(n, s);
    float a =  NoS / (ro * sqrt(1 - NoS*NoS) + SMALL_EPS);

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
    return F0 + (1 - F0) * pow((1 - max(0, dot(n, l) - SMALL_EPS)), 5);
}

float FresnelSchlick(vec3 v, vec3 l)
{
    vec3 h = (l + v) / length(l + v);
    return FresnelSchlick(FresnelSchlickF0(AirIOR, MarbleIOR), h, l);
}

//Cloth BRDF
float CharlieD(float roughness, float ndoth)
{
    float invR = 1. / roughness;
    float cos2h = ndoth * ndoth;
    float sin2h = 1. - cos2h;
    return (2. + invR) * pow(sin2h, invR * .5) / (2. * PI);
}

float AshikhminV(float ndotv, float ndotl)
{
    return 1. / (4. * (ndotl + ndotv - ndotl * ndotv));
}

float CookToranceLinen(vec3 v, vec3 n, vec3 l, float ro)
{
    vec3 h = (l + v) / length(l + v);

    float D = BeckmannNDF(n, h, ro);
    float G2 = SmithDirectionHeightCorrelatedMaskingShadowingFunction(
        BeckmannLambdaFunction(n, l, ro),
        BeckmannLambdaFunction(n, v, ro),
        GinnekenLambdaFunction(v, l)
    );
    float F = FresnelSchlick(FresnelSchlickF0(AirIOR, MarbleIOR), h, l);

    return F * CharlieD(ro, dot(n,h)) * AshikhminV(dot(n,v), dot(n,l)) * PI * dot(n, l);
}

//Cloth BRDF

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

    return (F * G2 * D) / (4 * max(abs(dot(n, l)), BIG_EPS) * max(abs(dot(n, v)), BIG_EPS));
}

vec3 ShirleyFresnelSubSurfaceAlbedo(float F0, vec3 ssAlbedo, vec3 v, vec3 n, vec3 l)
{
    return 21.f / (20.f * PI) * (1 - F0) * ssAlbedo
        * (1 - pow(1 - max(dot(n, l) - SMALL_EPS, 0), 5))
        * (1 - pow(1 - max(dot(n, v) - SMALL_EPS, 0), 5));
}

float LightFalloffWindowingFunction(float r0, float rMin, float rMax, float r)
{
    return pow(r0 / max(r, rMin), 2) * pow(max(1 - pow(r / rMax, 4), 0), 2);
}

vec3 CalculateRadiance(float shadowMapDepth, vec2 uv, vec3 n, vec3 shadowPosMVP, mat3 TBN)
{
    vec3 view = -normalize(positionWorld.xyz / positionWorld.w - uCameraPos);

    if (bool(uBumpMapAvailableUint) && bool(uBumpMappingEnabledUint)) {
        float h = AnisatropicTextureSample(uBumpMapSampler2D, uv).r;
        uv = uv + h * (TBN * view.xyz).xy * uBumpMapScaleFactorFloat;
    }
    vec3 ssAlbedo = AnisatropicTextureSample(uAlbedoMapSampler2D, uv).rgb;
    vec3 normal = normalize(TBN * normalize((AnisatropicTextureSample(uNormalMapSampler2D, uv).xyz * 2) - 1));
    float ro = bool(uRoughnessMapAvailableUint) ? AnisatropicTextureSample(uRoughnessSampler2D, uv).r : 1;
    vec3 radiance = ssAlbedo * 0.2f;

    if (bool(uPointLightEnabledUint))
    {
        for (uint i = 0; i < 5; i+=1)
        {
            vec3 pointLightDir = -normalize(positionWorld.xyz / positionWorld.w - uPointLightPosVec3Array[i]);
            vec3 fDiffPL = ShirleyFresnelSubSurfaceAlbedo(
                FresnelSchlickF0(AirIOR, MarbleIOR), ssAlbedo, view, normal, pointLightDir); //Shirley
            //vec3 fDiffPL = ssAlbedo / PI; //Lambert
            vec3 fSpecPL = PI *
                (uBrdfUint == 0 ? CookTorance(view, normal, pointLightDir, ro)
                    : CookToranceLinen(view, normal, pointLightDir, ro))
                * POINT_LIGHT_COLOR * max(dot(n, pointLightDir), 0);

            float d = distance(uPointLightPosVec3Array[i], positionWorld.xyz / positionWorld.w);
            radiance += LightFalloffWindowingFunction(1, 1, 350, d) * 100000 * (fSpecPL + fDiffPL);
        }
    }

    if (bool(uDirectLightEnabledUint))
    {
        vec3 fSpecDL = vec3(0);
        if (bool(uShadowMappingEnabledUint) && shadowPosMVP.z - 0.01f < shadowMapDepth)
        {
            fSpecDL = PI * (uBrdfUint == 0
                ? CookTorance(view, normal, directionalLightDir, ro)
                : CookToranceLinen(view, normal, directionalLightDir, ro))
                    * SUN_LIGHT_COLOR * max(dot(n, directionalLightDir), 0);

            radiance += (fSpecDL + ssAlbedo / PI);
        }
    }

    return radiance;
}

void main()
{
    outColor = vec4(1, 0, 0, 1);
    vec3 n = normalize(normalWorld);
    vec3 l = normalize(directionalLightDir);

    vec3 shadowPosMVP = positionShadowMapMvp.xyz / positionShadowMapMvp.w;
    float shadowMapDepth = texture(uShadowMapSampler2D, (shadowPosMVP.xy+1)*0.5f).x * 2 - 1;

    mat3 TBN = CalculateTBNMatrix(n, positionWorld.xyz / positionWorld.w, inUv);
    vec2 uv = inUv;

    if (uRenderModeUint == 0) // Full
    {
        if (bool(uDebugRenderModeAvailableUint)){
            outColor = vec4(uColor, 0.3f);
        }
        else{
            vec3 radiance = CalculateRadiance(shadowMapDepth, uv, n, shadowPosMVP, TBN);
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
        outColor = vec4(vec3(shadowPosMVP.z - 0.1f < shadowMapDepth ? 1 : 0), 1);
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
