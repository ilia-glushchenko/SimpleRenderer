/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <glbinding/Binding.h>
#include <glbinding/gl46/gl.h>
using namespace gl;

struct UniformBindingUI32
{
    int32_t location = -1;
    uint32_t const *data = nullptr;
};

struct UniformBindingF1
{
    int32_t location = -1;
    float const *data = nullptr;
};
using UniformBindingV3F = UniformBindingF1;
using UniformBindingM4F = UniformBindingF1;

struct UniformBindingUI32Array
{
    int32_t location = -1;
    uint32_t const *data = nullptr;
    uint32_t offset = 0;
    uint32_t stride = 0;
};

struct UniformBindingF1Array
{
    int32_t location = -1;
    float const *data = nullptr;
    uint32_t offset = 0;
    uint32_t stride = 0;
};
using UniformBindingV3FArray = UniformBindingF1Array;
using UniformBindingM4FArray = UniformBindingF1Array;

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
    std::vector<UniformBindingUI32> ui32;
    std::vector<UniformBindingF1> f1;
    std::vector<UniformBindingV3F> f3;
    std::vector<UniformBindingM4F> f16;

    std::vector<UniformBindingUI32Array> ui32Array;
    std::vector<UniformBindingF1Array> f1Array;
    std::vector<UniformBindingV3FArray> f3Array;
    std::vector<UniformBindingM4FArray> f16Array;

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

template <typename T>
struct GlobalUniformDescriptor
{
    std::vector<const char *> names;
    std::vector<T const *> data;
};

template <typename T>
struct LocalUniformDescriptor
{
    std::vector<const char *> names;
    std::vector<T const *> data;
    std::vector<uint32_t> offsets;
    std::vector<uint32_t> strides;
};

struct UniformsDescriptor
{
    using UI32 = GlobalUniformDescriptor<uint32_t>;
    using F1 = GlobalUniformDescriptor<float>;
    using F3 = GlobalUniformDescriptor<float>;
    using MAT4 = GlobalUniformDescriptor<float>;

    using ArrayUI32 = LocalUniformDescriptor<uint32_t>;
    using ArrayF1 = LocalUniformDescriptor<float>;
    using ArrayF3 = LocalUniformDescriptor<float>;
    using ArrayMAT4 = LocalUniformDescriptor<float>;

    GlobalUniformDescriptor<uint32_t> ui32;
    GlobalUniformDescriptor<float> float1;
    GlobalUniformDescriptor<float> float3;
    GlobalUniformDescriptor<float> mat4;

    LocalUniformDescriptor<uint32_t> ui32Array;
    LocalUniformDescriptor<float> float1Array;
    LocalUniformDescriptor<float> float3Array;
    LocalUniformDescriptor<float> mat4Array;
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
    std::vector<sr::math::Matrix4x4> prevModels;
    sr::math::Matrix4x4 prevView = sr::math::CreateIdentityMatrix();
    sr::math::Matrix4x4 prevProj = sr::math::CreateIdentityMatrix();
    sr::math::Matrix4x4 jitter = sr::math::CreateIdentityMatrix();
    uint32_t count = 0;
};

static const uint8_t RENDER_PASS_MAX_SUBPASS = 4;
static const uint8_t RENDER_PASS_MAX_DEPENDENCIES = 8;
static const uint8_t RENDER_PASS_MAX_ATTACHMENTS = 8;

struct SubPassDependencyDescriptor
{
    GLenum unit;
    GLenum texture;
    GLuint handle;
};

struct SubPassAttachmentDescriptor
{
    GLenum attachment;
    GLenum texture;
    GLuint handle;
};

struct SubPassDescriptor
{
    SubPassDependencyDescriptor dependencies[RENDER_PASS_MAX_DEPENDENCIES] = {};
    uint8_t dependencyCount = 0;
    SubPassAttachmentDescriptor attachments[RENDER_PASS_MAX_ATTACHMENTS] = {};
    uint8_t attachmentCount = 0;
};

struct SubPass
{
    SubPassDescriptor desc;
    GLuint fbo = 0;
    bool active = false;
};

struct RenderPass
{
    SubPass subPasses[RENDER_PASS_MAX_SUBPASS] = {};
    uint8_t subPassCount = 0;
    ShaderProgram program = {};
    int32_t width = 0;
    int32_t height = 0;
};

struct PipelineShaderPrograms
{
    ShaderProgram shadowMapping;
    ShaderProgram lighting;
    ShaderProgram velocity;
    ShaderProgram taa;
    ShaderProgram debug;
};

struct Pipeline
{
    RenderPass shadowMapping;
    RenderPass lighting;
    RenderPass velocity;
    RenderPass taa;
    RenderPass debug;
};
