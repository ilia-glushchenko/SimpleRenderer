/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "RenderDefinitions.hpp"
#include "Loader.hpp"

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

void CreateTextures(
    sr::load::TextureSource const &source,
    Texture2DDescriptor const &desc,
    GLuint *handles,
    uint32_t count)
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

GLuint CreateEmptyRGBATexture(int32_t width, int32_t height)
{
    return CreateTexture(sr::load::TextureSource{"", nullptr, width, height, 4, GL_RGBA});
}

GLuint CreateColorAttachment(int32_t width, int32_t height)
{
    GLuint handle = 0;

    auto const source = sr::load::TextureSource{"", nullptr, width, height, 4, GL_RGBA};
    auto desc = CreateDefaultTexture2DDescriptor(source);
    desc.internalFormat = GL_RGBA32F;

    CreateTextures(source, desc, &handle, 1);

    return handle;
}

void DeleteTexture(GLuint texture)
{
    glDeleteTextures(1, &texture);
}

void DeleteTextures(uint32_t count, GLuint *textures)
{
    glDeleteTextures(count, textures);
}
