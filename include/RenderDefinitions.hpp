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
    T *data;
    uint32_t count;
};
static_assert(std::is_pod<HeapArray<int>>::value, "HeapArray mush be a POD type.");

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
    GLuint vertexShaderHandle;
    GLuint fragmentShaderHandle;
    GLuint handle;
};
static_assert(std::is_pod<ShaderProgram>::value, "ShaderProgram must be a POD type.");

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
    SubPassDependencyDescriptor dependencies[RENDER_PASS_MAX_DEPENDENCIES];
    SubPassAttachmentDescriptor attachments[RENDER_PASS_MAX_ATTACHMENTS];

    uint8_t dependencyCount;
    uint8_t attachmentCount;

    bool enableWriteToDepth;
    bool enableClearDepthBuffer;
    uint8_t depthClearValue;
    GLenum depthTestFunction;

    bool enableWriteToColor;
    bool enableClearColorBuffer;
    uint8_t colorClearValue[4];

    void (*prePassCallback)();
    void (*postPassCallback)();
};
static_assert(std::is_pod<SubPassDescriptor>::value, "SubPassDescriptor must be a POD type.");

struct SubPass
{
    SubPassDescriptor desc;
    GLuint fbo;
    bool active;
};
static_assert(std::is_pod<SubPass>::value, "SubPass must be a POD type.");

constexpr uint8_t SHORT_STRING_MAX_LENGTH = 64;
struct ShortString
{
    char data[SHORT_STRING_MAX_LENGTH];
    uint8_t length;
};
static_assert(std::is_pod<ShortString>::value, "ShortString must be a POD type.");

struct RenderPass
{
    ShortString name;
    SubPass subPasses[RENDER_PASS_MAX_SUBPASS];
    ShaderProgram program;
    int32_t width;
    int32_t height;
    uint8_t subPassCount;
};
static_assert(std::is_pod<RenderPass>::value, "RenderPass must be a POD type.");

struct DeferredPipelineShaderPrograms
{
    ShaderProgram depthPrePass;
    ShaderProgram gBufferPass;
    ShaderProgram lighting;
    ShaderProgram debug;
};

constexpr uint8_t DefferPipelinePassCount = 4;
struct DeferredPipeline
{
    union {
        RenderPass passes[DefferPipelinePassCount];

        struct
        {
            RenderPass depthPrePass;
            RenderPass gBufferPass;
            RenderPass lighting;
            RenderPass debug;
        };
    };
};
static_assert((int)sizeof(DeferredPipeline) - (int)sizeof(DeferredPipeline::passes) == 0,
              "Number of named and unnamed render passes must be equal!");
static_assert(std::is_pod<DeferredPipeline>::value, "DeferredPipeline must be a POD type.");

struct ForwardPipelineShaderPrograms
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

constexpr uint8_t ForwardPipelinePassCount = 8;
struct ForwardPipeline
{
    union {
        RenderPass passes[ForwardPipelinePassCount];

        struct
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
    };
};
static_assert((int)sizeof(ForwardPipeline) - (int)sizeof(ForwardPipeline::passes) == 0,
              "Number of named and unnamed render passes must be equal!");
static_assert(std::is_pod<ForwardPipeline>::value, "ForwardPipeline must be a POD type.");

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

template <typename T>
struct SmallStackArray
{
    static constexpr uint8_t CAPACITY = 8;
    T data[CAPACITY] = {};
    uint8_t lenght = 0;
};

struct Technique
{
    SmallStackArray<float> dataF1 = {};
    SmallStackArray<ShortString> namesF1 = {};

    SmallStackArray<sr::math::Vec2> dataF2 = {};
    SmallStackArray<ShortString> namesF2 = {};

    SmallStackArray<sr::math::Vec3> dataF3 = {};
    SmallStackArray<ShortString> namesF3 = {};

    SmallStackArray<sr::math::Vec4> dataF4 = {};
    SmallStackArray<ShortString> namesF4 = {};

    SmallStackArray<sr::math::Matrix4x4> dataF16 = {};
    SmallStackArray<ShortString> namesF16 = {};
};
