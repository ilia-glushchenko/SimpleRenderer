/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "RenderDefinitions.hpp"
#include "Texture.hpp"

namespace
{

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
        glBufferData(GL_ARRAY_BUFFER, static_cast<size_t>(desc.count * desc.size), desc.data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    return vbos;
}

} // namespace

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