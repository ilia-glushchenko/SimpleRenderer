/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <glbinding/Binding.h>
#include <glbinding/gl46/gl.h>
using namespace gl;

struct UniformUi32
{
    std::string name;
    int32_t location = -1;
    uint32_t *data = nullptr;
};

struct Uniformf
{
    std::string name;
    int32_t location = -1;
    float *data = nullptr;
};
using UniformV3f = Uniformf;
using UniformM4f = Uniformf;

struct UniformTexture2D
{
    uint32_t handle = 0;
    uint32_t unit = 0;
};

struct Camera
{
    sr::math::Matrix4x4 proj = sr::math::CreateIdentityMatrix();
    sr::math::Matrix4x4 view = sr::math::CreateIdentityMatrix();
    sr::math::Vec3 pos = {};
    float yWorldAndle = 0;
    float near = 0;
    float far = 0;
    float fov = 0;
    float aspect = 0;
};

struct DirectionalLightSource
{
    sr::math::Matrix4x4 projection;
    sr::math::Matrix4x4 view;
};

struct RenderModel
{
    std::vector<uint32_t> vbos = {};
    GLuint indexBuffer = 0;
    GLuint vertexArrayObject = 0;
    GLuint albedoTexture = 0;
    GLuint normalTexture = 0;
    GLuint bumpTexture = 0;
    GLuint metallicTexture = 0;
    GLuint roughnessTexture = 0;
    uint32_t indexCount = 0;
    sr::math::Matrix4x4 model = sr::math::CreateIdentityMatrix();
    sr::math::Vec3 color = {0.5f, 0.5f, 0.5f};
};

struct ShaderProgram
{
    std::vector<UniformUi32> unifromsui32;
    std::vector<Uniformf> uniformsf;
    std::vector<UniformV3f> uniforms3f;
    std::vector<UniformM4f> uniforms16f;
    GLuint vertexShaderHandle = 0;
    GLuint fragmentShaderHandle = 0;
    GLuint handle = 0;
};

struct BufferDescriptor
{
    uint32_t size = 0;
    uint32_t count = 0;
    void *data = nullptr;
};

struct AttributeDescriptor
{
    std::string name;
    uint32_t dimensions = 0;
    uint32_t stride = 0;
};

struct UniformsDescriptor
{
    struct UINT32
    {
        std::vector<std::string> names;
        std::vector<uint32_t *> data;
    } ui32;

    struct FLOAT
    {
        std::vector<std::string> names;
        std::vector<float *> data;
    } float1;

    struct FLOAT3
    {
        std::vector<std::string> names;
        std::vector<float *> data;
    } float3;

    struct MAT4
    {
        std::vector<std::string> names;
        std::vector<float *> data;
    } mat4;
};

struct Texture2DDescriptor
{
    GLenum internalFormat;
    GLenum format;
    GLenum type;
    GLenum sWrap = GL_REPEAT;
    GLenum tWrap = GL_REPEAT;
    GLenum minFilter = GL_LINEAR;
    GLenum magFilter = GL_LINEAR;
};

struct TAABuffer
{
    sr::math::Matrix4x4 prevView = sr::math::CreateIdentityMatrix();
    sr::math::Matrix4x4 prevProj = sr::math::CreateIdentityMatrix();
    sr::math::Vec3 jitter = {};

    union {
        struct
        {
            GLuint drawTexture;
            GLuint historyTexture;
        };

        GLuint textures[2] = {};
    };

    uint32_t count = 0;
};

struct RenderPass
{
    ShaderProgram program = {};

    static const uint8_t MAX_DEPENDENCIES = 8;

    GLuint dependencies[MAX_DEPENDENCIES] = {};
    uint8_t dependencyCount = 0;

    int32_t width = 0;
    int32_t height = 0;

    const static uint32_t MAX_FBOS = 2;
    const static uint32_t MAX_TEXTURES = 2;

    union {
        struct
        {
            GLuint fbo1;
            GLuint fbo2;
        };

        GLuint fbos[MAX_FBOS] = {};
    };

    union {
        struct
        {
            GLuint colorTexture;
            GLuint depthTexture;
        };

        GLuint textures[MAX_TEXTURES] = {};
    };
};