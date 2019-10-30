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

RenderModel CreateRenderModel(RenderModelCreateInfo const &createInfo)
{
    RenderModel renderModel;
    renderModel.indexCount = createInfo.indexBufferDescriptor->count;

    renderModel.vbos = ::CreateBuffers(*createInfo.vertexBufferDescriptors);

    glGenVertexArrays(1, &renderModel.vertexArrayObject);
    glBindVertexArray(renderModel.vertexArrayObject);

    renderModel.indexBuffer = ::CreateBuffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderModel.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 createInfo.indexBufferDescriptor->size * createInfo.indexBufferDescriptor->count,
                 createInfo.indexBufferDescriptor->data,
                 GL_STATIC_DRAW);

    if (createInfo.material->albedo != nullptr)
    {
        renderModel.albedoTexture = CreateMipMappedTexture(*createInfo.material->albedo);
    }
    if (createInfo.material->normal != nullptr)
    {
        renderModel.normalTexture = CreateMipMappedTexture(*createInfo.material->normal);
    }
    if (createInfo.material->bump != nullptr)
    {
        renderModel.bumpTexture = CreateMipMappedTexture(*createInfo.material->bump);
    }
    if (createInfo.material->metallic != nullptr)
    {
        renderModel.metallicTexture = CreateMipMappedTexture(*createInfo.material->metallic);
    }
    if (createInfo.material->roughness != nullptr)
    {
        renderModel.roughnessTexture = CreateMipMappedTexture(*createInfo.material->roughness);
    }

    renderModel.model = createInfo.model;
    renderModel.color = createInfo.color;
    renderModel.center = sr::geo::CalculateCenterOfMass(
        createInfo.geometry->vertices.data(), createInfo.geometry->indices.data(), createInfo.geometry->indices.size());
    renderModel.aabb = sr::geo::CalculateAABB(
        createInfo.geometry->vertices.data(), createInfo.geometry->vertices.size(), renderModel.center, renderModel.model);

    renderModel.debugRenderModel = createInfo.debugRenderModel;
    renderModel.brdf = createInfo.material->brdf == "marbel" ? 0 : 1;

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
