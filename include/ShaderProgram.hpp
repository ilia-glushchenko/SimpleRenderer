/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "RenderDefinitions.hpp"
#include "Loader.hpp"

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

template <typename T, typename D>
std::vector<T> CreateUniformBinding(GLuint program, std::vector<D *> const &data, std::vector<const char *> const &names, std::vector<int32_t> const &counts)
{
    assert(data.size() == names.size());
    assert(data.size() == counts.size());

    std::vector<T> bindings;

    for (uint32_t i = 0; i < names.size(); ++i)
    {
        assert(counts[i] > 0);
        bindings.push_back({static_cast<int32_t>(glGetUniformLocation(program, names[i])), data[i], counts[i]});

        if (bindings.back().location == -1)
        {
            std::cerr << "Failed to get uniform location! index: " << i << "; name: '" << names[i] << "'" << std::endl;
        }
    }

    return bindings;
}

template <typename T, typename D>
std::vector<T> CreateUniformBinding(
    GLuint program,
    std::vector<D *> const &data,
    std::vector<const char *> const &names,
    std::vector<uint32_t> const &offsets,
    std::vector<uint32_t> const &strides)
{
    assert(data.size() == names.size());
    assert(data.size() == offsets.size());
    assert(data.size() == strides.size());

    std::vector<T> bindings;

    for (uint32_t i = 0; i < names.size(); ++i)
    {
        bindings.push_back({glGetUniformLocation(program, names[i]),
                            data[i],
                            offsets[i],
                            strides[i]});

        if (bindings.back().location == -1)
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
    program.perFrameUniformBindings.UI32 = CreateUniformBinding<UniformBindingUI32>(program.handle, desc.ui32.data, desc.ui32.names, desc.ui32.counts);
    program.perFrameUniformBindings.Float1 = CreateUniformBinding<UniformBindingF1>(program.handle, desc.float1.data, desc.float1.names, desc.float1.counts);
    program.perFrameUniformBindings.Float2 = CreateUniformBinding<UniformBindingV2F>(program.handle, desc.float2.data, desc.float2.names, desc.float2.counts);
    program.perFrameUniformBindings.Float3 = CreateUniformBinding<UniformBindingV3F>(program.handle, desc.float3.data, desc.float3.names, desc.float3.counts);
    program.perFrameUniformBindings.Float4 = CreateUniformBinding<UniformBindingV4F>(program.handle, desc.float4.data, desc.float4.names, desc.float4.counts);
    program.perFrameUniformBindings.Float16 = CreateUniformBinding<UniformBindingM4F>(program.handle, desc.mat4.data, desc.mat4.names, desc.mat4.counts);
    program.perModelUniformBindings.UI32 = CreateUniformBinding<UniformBindingUI32Array>(program.handle, desc.ui32Array.data, desc.ui32Array.names, desc.ui32Array.offsets, desc.ui32Array.strides);
    program.perModelUniformBindings.Float1 = CreateUniformBinding<UniformBindingF1Array>(program.handle, desc.float1Array.data, desc.float1Array.names, desc.float1Array.offsets, desc.float1Array.strides);
    program.perModelUniformBindings.Float2 = CreateUniformBinding<UniformBindingV2FArray>(program.handle, desc.float2Array.data, desc.float2Array.names, desc.float2Array.offsets, desc.float2Array.strides);
    program.perModelUniformBindings.Float3 = CreateUniformBinding<UniformBindingV3FArray>(program.handle, desc.float3Array.data, desc.float3Array.names, desc.float3Array.offsets, desc.float3Array.strides);
    program.perModelUniformBindings.Float4 = CreateUniformBinding<UniformBindingV4FArray>(program.handle, desc.float4Array.data, desc.float4Array.names, desc.float4Array.offsets, desc.float4Array.strides);
    program.perModelUniformBindings.Float16 = CreateUniformBinding<UniformBindingM4FArray>(program.handle, desc.mat4Array.data, desc.mat4Array.names, desc.mat4Array.offsets, desc.mat4Array.strides);
}

void DeleteShaderProgram(ShaderProgram &program)
{
    glDeleteProgram(program.handle);
    glDeleteShader(program.vertexShaderHandle);
    glDeleteShader(program.fragmentShaderHandle);
    ShaderProgram emptyProgram;
    std::swap(program, emptyProgram);
}
