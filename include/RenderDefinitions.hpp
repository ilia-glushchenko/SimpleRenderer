/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "Geometry.hpp"
#include "Math.hpp"

#include <glbinding/Binding.h>
#include <glbinding/gl46ext/gl.h>
using namespace gl;

namespace sr::load
{
struct Geometry;
struct MaterialSource;
} // namespace sr::load

struct UniformBindingUI32
{
    int32_t location = -1;
    uint32_t const *data = nullptr;
    int32_t count = 0;
};

struct UniformBindingF1
{
    int32_t location = -1;
    float const *data = nullptr;
    int32_t count = 0;
};
using UniformBindingV2F = UniformBindingF1;
using UniformBindingV3F = UniformBindingF1;
using UniformBindingV4F = UniformBindingF1;
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
using UniformBindingV2FArray = UniformBindingF1Array;
using UniformBindingV3FArray = UniformBindingF1Array;
using UniformBindingV4FArray = UniformBindingF1Array;
using UniformBindingM4FArray = UniformBindingF1Array;

template <typename T>
struct FrameUniformDescriptor
{
    std::vector<const char *> names;
    std::vector<T const *> data;
    std::vector<int32_t> counts;
};

template <typename T>
struct ModelUniformDescriptor
{
    std::vector<const char *> names;
    std::vector<T const *> data;
    std::vector<uint32_t> offsets;
    std::vector<uint32_t> strides;
};

struct UniformsDescriptor
{
    using PerFrameUI32 = FrameUniformDescriptor<uint32_t>;
    using PerFrameFloat1 = FrameUniformDescriptor<float>;
    using PerFrameFloat2 = FrameUniformDescriptor<float>;
    using PerFrameFloat3 = FrameUniformDescriptor<float>;
    using PerFrameFloat4 = FrameUniformDescriptor<float>;
    using PerFrameMat4 = FrameUniformDescriptor<float>;

    using PerModelUI32 = ModelUniformDescriptor<uint32_t>;
    using PerModelFloat1 = ModelUniformDescriptor<float>;
    using PerModelFloat2 = ModelUniformDescriptor<float>;
    using PerModelFloat3 = ModelUniformDescriptor<float>;
    using PerModelFloat4 = ModelUniformDescriptor<float>;
    using PerModelMat4 = ModelUniformDescriptor<float>;

    FrameUniformDescriptor<uint32_t> ui32;
    FrameUniformDescriptor<float> float1;
    FrameUniformDescriptor<float> float2;
    FrameUniformDescriptor<float> float3;
    FrameUniformDescriptor<float> float4;
    FrameUniformDescriptor<float> mat4;

    ModelUniformDescriptor<uint32_t> ui32Array;
    ModelUniformDescriptor<float> float1Array;
    ModelUniformDescriptor<float> float2Array;
    ModelUniformDescriptor<float> float3Array;
    ModelUniformDescriptor<float> float4Array;
    ModelUniformDescriptor<float> mat4Array;
};

template <typename T>
struct HeapArray
{
    T *data = nullptr;
    uint32_t count = 0;
};

struct PerFrameUniformBindings
{
    HeapArray<UniformBindingUI32> UI32;
    HeapArray<UniformBindingF1> Float1;
    HeapArray<UniformBindingV2F> Float2;
    HeapArray<UniformBindingV3F> Float3;
    HeapArray<UniformBindingV4F> Float4;
    HeapArray<UniformBindingM4F> Float16;
};

struct PerModleUniformBindings
{
    HeapArray<UniformBindingUI32Array> UI32;
    HeapArray<UniformBindingF1Array> Float1;
    HeapArray<UniformBindingV2FArray> Float2;
    HeapArray<UniformBindingV3FArray> Float3;
    HeapArray<UniformBindingV4FArray> Float4;
    HeapArray<UniformBindingM4FArray> Float16;
};

struct ShaderProgram
{
    PerFrameUniformBindings perFrameUniformBindings;
    PerModleUniformBindings perModelUniformBindings;
    GLuint vertexShaderHandle = 0;
    GLuint fragmentShaderHandle = 0;
    GLuint handle = 0;
};

struct AttributeDescriptor
{
    std::string name;
    uint32_t dimensions = 0;
    uint32_t stride = 0;
};

struct BufferDescriptor
{
    uint32_t size = 0;
    uint32_t count = 0;
    void *data = nullptr;
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
    float maxAnisatropy = 1.f;
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
    sr::math::Vec4 clearColor = {0, 0, 0, 0};
    float clearDepth = 1.0f;
    ClearBufferMask clearBufferMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    GLboolean depthMask = GL_TRUE;
    GLenum depthFunc = GL_LESS;
};

struct SubPass
{
    SubPassDescriptor desc;
    GLuint fbo = 0;
    bool active = false;
};

constexpr uint8_t SHORT_STRING_MAX_LENGTH = 64;
struct ShortString
{
    char data[SHORT_STRING_MAX_LENGTH] = {};
    uint8_t length = 0;
};

struct RenderPass
{
    ShortString name;
    SubPass subPasses[RENDER_PASS_MAX_SUBPASS] = {};
    ShaderProgram program = {};
    int32_t width = 0;
    int32_t height = 0;
    uint8_t subPassCount = 0;
};

struct PipelineShaderPrograms
{
    ShaderProgram depthPrePass;
    ShaderProgram shadowMapping;
    ShaderProgram lighting;
    ShaderProgram transparent;
    ShaderProgram velocity;
    ShaderProgram taa;
    ShaderProgram toneMapping;
    ShaderProgram debug;
};

struct Pipeline
{
    RenderPass depthPrePass;
    RenderPass shadowMapping;
    RenderPass lighting;
    RenderPass transparent;
    RenderPass velocity;
    RenderPass taa;
    RenderPass toneMapping;
    RenderPass debug;
};

struct TAABuffer
{
    sr::math::Matrix4x4 projUnjit = sr::math::CreateIdentityMatrix();
    sr::math::Matrix4x4 prevProjUnjit = sr::math::CreateIdentityMatrix();
    std::vector<sr::math::Matrix4x4> prevModels;
    sr::math::Vec2 jitter = {};
    uint32_t count = 0;
};

struct Camera
{
    sr::math::Matrix4x4 proj = sr::math::CreateIdentityMatrix();
    sr::math::Matrix4x4 view = sr::math::CreateIdentityMatrix();
    sr::math::Vec3 pos = {};
    float xWorldAngle = 0;
    float yWorldAngle = 0;
    float near = 0;
    float far = 0;
    float fov = 0;
    float aspect = 0;
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
    uint32_t indexCount = 0; //The amount of indices in the index buffer
    sr::math::Matrix4x4 model = sr::math::CreateIdentityMatrix();
    sr::math::Vec3 color = {0.5f, 0.5f, 0.5f};
    sr::math::Vec3 center = {};
    sr::geo::AABB aabb = {};
    GLuint debugRenderModel = 0; //This model is for debug rendering only
    //ToDo: Better material system is needed
    GLuint brdf = 0;
};

struct RenderModelCreateInfo
{
    sr::load::Geometry const *geometry = nullptr;
    std::vector<BufferDescriptor> const *vertexBufferDescriptors = nullptr;
    BufferDescriptor const *indexBufferDescriptor = nullptr;
    sr::load::MaterialSource const *material = nullptr;
    sr::math::Vec3 position = {0, 0, 0};
    sr::math::Vec3 orientation = {0, 0, 0};
    sr::math::Vec3 scale = {1, 1, 1};
    sr::math::Vec3 color;
    GLuint debugRenderModel;
};

struct OrthographicFrustum
{
    float left;
    float right;
    float bottom;
    float top;
    float near;
    float far;
};

struct DirectionalLightSource
{
    sr::math::Vec3 position = {0, 10000, 0};
    sr::math::Vec3 orientation = {-1.5f, -0.5f, 0};
    OrthographicFrustum frustum = {-2048.f, 2048.f, -2048.f, 2048.f, -2000.f, 1500.f};
    sr::math::Matrix4x4 projection = sr::math::CreateIdentityMatrix();
    sr::math::Matrix4x4 view = sr::math::CreateIdentityMatrix();
    float radiantFlux = 1.5f;
};

struct ShadowingMaskingFunction
{

};
