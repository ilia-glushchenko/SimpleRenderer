/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "TestModels.hpp"
#include "RendererDefinitions.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <ctime>
#include <cassert>
#include <unordered_map>

namespace
{
void GLAPIENTRY
MessageCallback(GLenum source,
                GLenum type,
                GLuint id,
                GLenum severity,
                GLsizei length,
                const GLchar *message,
                const void *userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

GLuint CreateShader(GLenum shaderType, const char *code)
{
    if (code == nullptr)
    {
        std::cerr << "Failed to create shader! Code is invalid" << std::endl;
    }

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == 0)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        std::cerr << "Failed to create shader! Compilation failed:\n"
                  << errorLog.data() << std::endl;

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

GLuint CreateShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
    if (vertexShader == 0 || fragmentShader == 0)
    {
        std::cerr << "Failed to create shader program! "
                  << (vertexShader == 0 ? "Vertex" : "Fragment")
                  << " shader is not valid" << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, fragmentShader);
    glAttachShader(program, vertexShader);
    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == 0)
    {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        std::cerr << "Failed to create shader program! Linkage failed:\n"
                  << infoLog.data();

        return 0;
    }

    return program;
}

GLint CreateBuffer()
{
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    if (buffer == 0)
    {
        std::cerr << "Failed to create buffer object!" << std::endl;
        return {};
    }

    return buffer;
}

std::vector<uint32_t> CreateBuffers(std::vector<BufferDescriptor> const &bufferDescriptors)
{
    std::vector<uint32_t> vbos;

    for (auto &desc : bufferDescriptors)
    {
        vbos.push_back(CreateBuffer());
        glBindBuffer(GL_ARRAY_BUFFER, vbos.back());
        glBufferData(GL_ARRAY_BUFFER, desc.count * desc.size, desc.data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    return vbos;
}

template <typename T, typename D>
std::vector<T> GetUniformLocations(GLuint program, std::vector<D *> const &data, std::vector<std::string> const &names)
{
    assert(data.size() == names.size());

    std::vector<T> uniforms;

    for (uint32_t i = 0; i < names.size(); ++i)
    {
        uniforms.push_back({names[i],
                            glGetUniformLocation(program, names[i].c_str()),
                            data[i]});

        if (uniforms.back().location == -1)
        {
            std::cerr << "Failed to get uniform location! index: " << i << "; name: '" << names[i] << "'" << std::endl;
        }
    }

    return uniforms;
}
} // namespace

ShaderProgram CreateShaderProgram(char const *vert, char const *frag, UniformsDescriptor const &uniformsDescriptor)
{
    auto const vertexShaderHandle = ::CreateShader(GL_VERTEX_SHADER, sr::load::LoadFile(vert).c_str());
    auto const fragmentShaderHandle = ::CreateShader(GL_FRAGMENT_SHADER, sr::load::LoadFile(frag).c_str());
    auto const program = CreateShaderProgram(vertexShaderHandle, fragmentShaderHandle);
    if (program != 0)
    {
        std::time_t const timestamp = std::time(nullptr);
        std::cout << "Shader program created:\n"
                  << "Vertex   shader: " << vert << "\n"
                  << "Fragment shader: " << frag << "\n"
                  << std::asctime(std::localtime(&timestamp)) << std::endl;
    }

    return ShaderProgram{
        GetUniformLocations<UniformUi32>(program, uniformsDescriptor.ui32.data, uniformsDescriptor.ui32.names),
        GetUniformLocations<Uniformf>(program, uniformsDescriptor.float1.data, uniformsDescriptor.float1.names),
        GetUniformLocations<UniformV3f>(program, uniformsDescriptor.float3.data, uniformsDescriptor.float3.names),
        GetUniformLocations<UniformM4f>(program, uniformsDescriptor.mat4.data, uniformsDescriptor.mat4.names),
        vertexShaderHandle,
        fragmentShaderHandle,
        program};
}

void DeleteShaderProgram(ShaderProgram &program)
{
    glDeleteProgram(program.handle);
    glDeleteShader(program.vertexShaderHandle);
    glDeleteShader(program.fragmentShaderHandle);
    ShaderProgram emptyProgram;
    std::swap(program, emptyProgram);
}

Camera CreateCamera()
{
    Camera camera;

    camera.fov = 1.0472f;
    camera.aspect = 1;
    camera.near = 0.1f;
    camera.far = 10000.f;
    camera.proj = sr::math::CreatePerspectiveProjectionMatrix(camera.near, camera.far, camera.fov, 1.f);
    camera.view = sr::math::CreateIdentityMatrix();
    camera.pos = {-1217.f, 101.f, 40.f};
    camera.yWorldAndle = -1.46f;

    return camera;
}

Texture2DDescriptor CreateDefaultTexture2DDescriptor(sr::load::TextureSource const &source)
{
    Texture2DDescriptor desc;

    desc.sWrap = GL_REPEAT;
    desc.tWrap = GL_REPEAT;
    desc.minFilter = GL_NEAREST;
    desc.magFilter = GL_NEAREST;
    desc.internalFormat = source.format;
    desc.format = source.format;
    desc.type = GL_UNSIGNED_BYTE;

    return desc;
}

GLuint InitializeTexture(Texture2DDescriptor const &desc, sr::load::TextureSource const &source)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, desc.sWrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, desc.tWrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc.magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc.minFilter);

    glTexImage2D(GL_TEXTURE_2D, 0, desc.internalFormat,
                 source.width, source.height, 0, desc.format,
                 desc.type, source.data);

    return 0;
}

void CreateTextures(sr::load::TextureSource const &source, Texture2DDescriptor const &desc, GLuint *handles, uint32_t count)
{
    glGenTextures(count, handles);

    for (uint32_t i = 0; i < count; ++i)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, handles[i]);
        InitializeTexture(desc, source);
    }
}

GLuint CreateTexture(sr::load::TextureSource const &source)
{
    GLuint handle = 0;

    CreateTextures(source, CreateDefaultTexture2DDescriptor(source), &handle, 1);

    return handle;
}

GLuint CreateMipMappedTexture(sr::load::TextureSource const &source)
{
    static std::unordered_map<uint8_t const *, GLuint> s_textureCache(11);

    auto cachedTextureIt = s_textureCache.find(source.data);
    if (cachedTextureIt != s_textureCache.end())
    {
        return cachedTextureIt->second;
    }

    GLuint &handle = s_textureCache[source.data];
    handle = CreateTexture(source);

    if (source.data != nullptr)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return handle;
}

GLuint CreateDepthTexture(uint32_t width, uint32_t height)
{
    GLuint depthTexture = 0;

    glGenTextures(1, &depthTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    Texture2DDescriptor desc;
    desc.sWrap = GL_CLAMP_TO_EDGE;
    desc.tWrap = GL_CLAMP_TO_EDGE;
    desc.minFilter = GL_NEAREST;
    desc.magFilter = GL_NEAREST;
    desc.internalFormat = GL_DEPTH_COMPONENT32;
    desc.format = GL_DEPTH_COMPONENT;
    desc.type = GL_FLOAT;

    sr::load::TextureSource source;
    source.width = width;
    source.height = height;
    source.data = nullptr;

    InitializeTexture(desc, source);

    return depthTexture;
}

void DeleteTexture(GLuint texture)
{
    glDeleteTextures(1, &texture);
}

void DeleteTextures(uint32_t count, GLuint *textures)
{
    glDeleteTextures(count, textures);
}

RenderModel CreateRenderModel(
    std::vector<BufferDescriptor> const &bufferDescriptors,
    BufferDescriptor const &indexBufferDescriptor,
    sr::load::MaterialSource const &material)
{
    RenderModel renderModel;
    renderModel.indexCount = indexBufferDescriptor.count;

    renderModel.vbos = ::CreateBuffers(bufferDescriptors);

    glGenVertexArrays(1, &renderModel.vertexArrayObject);
    glBindVertexArray(renderModel.vertexArrayObject);

    renderModel.indexBuffer = ::CreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderModel.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferDescriptor.size * indexBufferDescriptor.count, indexBufferDescriptor.data, GL_STATIC_DRAW);

    if (material.albedo != nullptr)
    {
        renderModel.albedoTexture = CreateMipMappedTexture(*material.albedo);
    }
    if (material.normal != nullptr)
    {
        renderModel.normalTexture = CreateMipMappedTexture(*material.normal);
    }
    if (material.bump != nullptr)
    {
        renderModel.bumpTexture = CreateMipMappedTexture(*material.bump);
    }
    if (material.metallic != nullptr)
    {
        renderModel.metallicTexture = CreateMipMappedTexture(*material.metallic);
    }
    if (material.roughness != nullptr)
    {
        renderModel.roughnessTexture = CreateMipMappedTexture(*material.roughness);
    }

    return renderModel;
}

void LinkRenderModelToShaderProgram(
    GLuint program,
    RenderModel const &model,
    std::vector<AttributeDescriptor> const &attribs)
{
    assert(attribs.size() == model.vbos.size());

    for (uint32_t i = 0; i < attribs.size(); ++i)
    {
        int32_t const location = glGetAttribLocation(program, attribs[i].name.c_str());
        glBindBuffer(GL_ARRAY_BUFFER, model.vbos[i]);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, attribs[i].dimensions, GL_FLOAT, GL_FALSE, attribs[i].stride, reinterpret_cast<void *>(0));
    }
}

TAABuffer CreateTAABuffer(int32_t width, int32_t height)
{
    sr::load::TextureSource const source{"", nullptr, width, height, 4, GL_RGBA};

    TAABuffer taaBuffer;
    taaBuffer.count = 0;
    CreateTextures(source, CreateDefaultTexture2DDescriptor(source), taaBuffer.textures, 2);

    return taaBuffer;
}

void DeleteTAABuffer(TAABuffer &buffer)
{
    DeleteTextures(1, &buffer.historyTexture);
    DeleteTextures(1, &buffer.drawTexture);
}

GLFWwindow *InitializeGLFW(uint32_t width, uint32_t height)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

    GLFWwindow *window = glfwCreateWindow(width, height, "Simple Renderer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glbinding::Binding::initialize(glfwGetProcAddress);

    return window;
}

void InitializeImGui(GLFWwindow *window)
{
    const char *glsl_version = "#version 460";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    auto &io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
}

void ConfigureGL()
{
    //glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(::MessageCallback, 0);
    glClearColor(1, 1, 1, 1);
    glClearDepth(1.0f);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
}

void InitializeGlobals()
{
    g_quadWallRenderModel = CreateRenderModel(sr::load::CreateBufferDescriptors(g_quadWall),
                                              sr::load::CreateIndexBufferDescriptor(g_quadWall),
                                              {});
}

void DeinitializeImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DeinitializeGLFW(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
