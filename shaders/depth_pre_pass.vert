#version 460

layout (location = 10) uniform mat4 uProjMat;
layout (location = 11) uniform mat4 uProjUnjitMat;
layout (location = 12) uniform mat4 uViewMat;
layout (location = 13) uniform mat4 uModelMat;
layout (location = 15) uniform uint uTaaJitterEnabledUint;

layout (location = 0) in vec3 aPosition;

void main()
{
    mat4 projection = bool(uTaaJitterEnabledUint) ? uProjMat : uProjUnjitMat;
    gl_Position = projection * uViewMat * uModelMat * vec4(aPosition, 1);
}
