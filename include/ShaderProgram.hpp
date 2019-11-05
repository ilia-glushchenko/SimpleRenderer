/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "Loader.hpp"
#include "RenderDefinitions.hpp"

#include <ctime>
#include <iostream>

namespace
{

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

template <typename T>
HeapArray<T> AllocateHeapArray(uint32_t count)
{
    return count > 0 ? HeapArray<T>{reinterpret_cast<T*>(std::malloc(sizeof(T)* count)), count} : HeapArray<T>{};
}

template <typename T>
void FreeHeapArray(HeapArray<T> &heapArray)
{
    assert((heapArray.data != nullptr && heapArray.count > 0) || heapArray.data == nullptr && heapArray.count == 0);

    if (heapArray.data != nullptr)
    {
        std::free(heapArray.data);
        heapArray.data = nullptr;
        heapArray.count = 0;
    }
}

template <typename T, typename D>
HeapArray<T> CreateUniformBindings(GLuint program, std::vector<D *> const &data, std::vector<const char *> const &names, std::vector<int32_t> const &counts)
{
    assert(data.size() == names.size());
    assert(data.size() == counts.size());

    HeapArray<T> bindings = AllocateHeapArray<T>(static_cast<uint32_t>(data.size()));

    for (uint32_t i = 0; i < names.size(); ++i)
    {
        assert(counts[i] > 0);
        bindings.data[i] = T{static_cast<int32_t>(glGetUniformLocation(program, names[i])), data[i], counts[i]};

        if (bindings.data[i].location == -1)
        {
            std::cerr << "Failed to get uniform location! index: " << i << "; name: '" << names[i] << "'" << std::endl;
        }
    }

    return bindings;
}

template <typename T, typename D>
HeapArray<T> CreateUniformBindings(
    GLuint program,
    std::vector<D *> const &data,
    std::vector<const char *> const &names,
    std::vector<uint32_t> const &offsets,
    std::vector<uint32_t> const &strides)
{
    assert(data.size() == names.size());
    assert(data.size() == offsets.size());
    assert(data.size() == strides.size());

    HeapArray<T> bindings = AllocateHeapArray<T>(static_cast<uint32_t>(data.size()));

    for (uint32_t i = 0; i < names.size(); ++i)
    {
        bindings.data[i] = T{glGetUniformLocation(program, names[i]),
                             data[i],
                             offsets[i],
                             strides[i]};

        if (bindings.data[i].location == -1)
        {
            std::cerr << "Failed to get uniform location! index: " << i << "; name: '" << names[i] << "'" << std::endl;
        }
    }

    return bindings;
}

} // namespace

ShaderProgram CreateShaderProgram(char const *vert, char const *frag)
{
    ShaderProgram program;

    program.vertexShaderHandle = ::CreateShader(GL_VERTEX_SHADER, sr::load::LoadFile(vert).c_str());
    program.fragmentShaderHandle = ::CreateShader(GL_FRAGMENT_SHADER, sr::load::LoadFile(frag).c_str());
    program.handle = CreateShaderProgram(program.vertexShaderHandle, program.fragmentShaderHandle);
    if (program.handle != 0)
    {
        std::time_t const timestamp = std::time(nullptr);
        std::cout << "Shader program created:\n"
                  << "Vertex   shader: " << vert << "\n"
                  << "Fragment shader: " << frag << "\n"
                  << std::asctime(std::localtime(&timestamp)) << std::endl;
    }

    return program;
}

void CreateShaderProgramUniformBindings(ShaderProgram &program, UniformsDescriptor const &desc)
{
    program.perFrameUniformBindings.UI32 = CreateUniformBindings<UniformBindingUI32>(program.handle, desc.ui32.data, desc.ui32.names, desc.ui32.counts);
    program.perFrameUniformBindings.Float1 = CreateUniformBindings<UniformBindingF1>(program.handle, desc.float1.data, desc.float1.names, desc.float1.counts);
    program.perFrameUniformBindings.Float2 = CreateUniformBindings<UniformBindingV2F>(program.handle, desc.float2.data, desc.float2.names, desc.float2.counts);
    program.perFrameUniformBindings.Float3 = CreateUniformBindings<UniformBindingV3F>(program.handle, desc.float3.data, desc.float3.names, desc.float3.counts);
    program.perFrameUniformBindings.Float4 = CreateUniformBindings<UniformBindingV4F>(program.handle, desc.float4.data, desc.float4.names, desc.float4.counts);
    program.perFrameUniformBindings.Float16 = CreateUniformBindings<UniformBindingM4F>(program.handle, desc.mat4.data, desc.mat4.names, desc.mat4.counts);
    program.perModelUniformBindings.UI32 = CreateUniformBindings<UniformBindingUI32Array>(program.handle, desc.ui32Array.data, desc.ui32Array.names, desc.ui32Array.offsets, desc.ui32Array.strides);
    program.perModelUniformBindings.Float1 = CreateUniformBindings<UniformBindingF1Array>(program.handle, desc.float1Array.data, desc.float1Array.names, desc.float1Array.offsets, desc.float1Array.strides);
    program.perModelUniformBindings.Float2 = CreateUniformBindings<UniformBindingV2FArray>(program.handle, desc.float2Array.data, desc.float2Array.names, desc.float2Array.offsets, desc.float2Array.strides);
    program.perModelUniformBindings.Float3 = CreateUniformBindings<UniformBindingV3FArray>(program.handle, desc.float3Array.data, desc.float3Array.names, desc.float3Array.offsets, desc.float3Array.strides);
    program.perModelUniformBindings.Float4 = CreateUniformBindings<UniformBindingV4FArray>(program.handle, desc.float4Array.data, desc.float4Array.names, desc.float4Array.offsets, desc.float4Array.strides);
    program.perModelUniformBindings.Float16 = CreateUniformBindings<UniformBindingM4FArray>(program.handle, desc.mat4Array.data, desc.mat4Array.names, desc.mat4Array.offsets, desc.mat4Array.strides);
}

void DeleteShaderProgramUniformBindings(ShaderProgram &program)
{
    FreeHeapArray(program.perFrameUniformBindings.UI32);
    FreeHeapArray(program.perFrameUniformBindings.Float1);
    FreeHeapArray(program.perFrameUniformBindings.Float2);
    FreeHeapArray(program.perFrameUniformBindings.Float3);
    FreeHeapArray(program.perFrameUniformBindings.Float4);
    FreeHeapArray(program.perFrameUniformBindings.Float16);
    FreeHeapArray(program.perModelUniformBindings.UI32);
    FreeHeapArray(program.perModelUniformBindings.Float1);
    FreeHeapArray(program.perModelUniformBindings.Float2);
    FreeHeapArray(program.perModelUniformBindings.Float3);
    FreeHeapArray(program.perModelUniformBindings.Float4);
    FreeHeapArray(program.perModelUniformBindings.Float16);
}

void DeleteShaderProgram(ShaderProgram &program)
{
    glDeleteProgram(program.handle);
    glDeleteShader(program.vertexShaderHandle);
    glDeleteShader(program.fragmentShaderHandle);
    DeleteShaderProgramUniformBindings(program);
    ShaderProgram emptyProgram;
    std::swap(program, emptyProgram);
}
