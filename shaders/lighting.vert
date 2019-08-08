#version 460

layout (location = 10) uniform vec3 uColor;
layout (location = 11) uniform mat4 uProjMat;
layout (location = 12) uniform mat4 uViewMat;
layout (location = 13) uniform mat4 uModelMat;
layout (location = 14) uniform mat4 uDirLightProjMat;
layout (location = 15) uniform mat4 uDirLightViewMat;

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

layout (location = 0) out vec4 positionWorld;
layout (location = 1) out vec4 positionView;
layout (location = 2) out vec3 normalWorld;
layout (location = 3) out vec3 normalView;
layout (location = 4) out vec3 directionalLightDir;
layout (location = 5) out vec4 positionShadowMapMvp;
layout (location = 6) out vec2 uv;

void main()
{
    positionWorld = uModelMat * vec4(aPosition, 1);
    positionView = uViewMat * uModelMat * vec4(aPosition, 1);
    normalWorld = (uModelMat * vec4(aNormal, 0)).xyz;
    normalView = (uViewMat * uModelMat * vec4(aNormal, 0)).xyz;
    directionalLightDir = (uViewMat * normalize(vec4(0, 1, 0, 0))).xyz;

    positionShadowMapMvp = uDirLightProjMat * uDirLightViewMat * uModelMat * vec4(aPosition, 1.0);

    uv = aUV;

    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPosition, 1);
}
