/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "RenderDefinitions.hpp"
#include "Math.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

namespace
{
struct VNTM
{
    int v, n, t, m;
    bool operator==(VNTM const &o) const
    {
        return v == o.v && n == o.n && t == o.t && o.m == m;
    }
};

struct VNTMhasher
{
    std::size_t operator()(const VNTM &k) const
    {
        return k.v ^ k.n ^ k.t ^ k.m;
    }
};
} // namespace

namespace sr::load
{
struct Geometry
{
    std::vector<math::Vec3> vertices;
    std::vector<math::Vec3> normals;
    std::vector<math::Vec2> uvs;
    std::vector<uint32_t> indices;
    uint32_t material = 0;
};

struct TextureSource
{
    std::string filepath;
    uint8_t *data = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;
    GLenum format = GL_NONE;
};

struct MaterialSource
{
    TextureSource *albedo;
    TextureSource *normal;
    TextureSource *bump;
    TextureSource *metallic;
    TextureSource *roughness;
};

struct Model
{
    Geometry geometry;
    MaterialSource material;
};

void LoadGeometry(tinyobj::attrib_t const &attrib, tinyobj::shape_t const &shape, std::vector<Geometry> &geometries)
{
    std::unordered_map<::VNTM, size_t, ::VNTMhasher> vertexMapping;
    std::vector<uint32_t> uniqueMaterials;
    std::vector<uint32_t> geometryIndices;
    auto &mesh = shape.mesh;

    for (size_t i = 0; i < mesh.indices.size(); ++i)
    {
        uint32_t const index = static_cast<uint32_t>(std::floor(i / 3.f));
        int const materialId = mesh.material_ids[index];
        ::VNTM const key = {
            mesh.indices[i].vertex_index,
            mesh.indices[i].normal_index,
            mesh.indices[i].texcoord_index,
            materialId};

        uint32_t uniqueIndex = 0;
        Geometry *geometry = nullptr;
        if (auto it = std::find(uniqueMaterials.begin(), uniqueMaterials.end(), materialId); it == uniqueMaterials.end())
        {
            uniqueIndex = static_cast<uint32_t>(uniqueMaterials.size());

            uniqueMaterials.push_back(static_cast<uint32_t>(materialId));
            geometryIndices.push_back(static_cast<uint32_t>(geometries.size()));

            geometries.resize(geometries.size() + 1);
            geometry = &geometries.back();
            geometry->material = materialId;
        }
        else
        {
            geometry = &geometries[geometryIndices[uniqueIndex]];
        }

        if (auto it = vertexMapping.find(key); it == vertexMapping.end())
        {
            vertexMapping[key] = geometry->vertices.size();

            geometry->indices.push_back(static_cast<uint32_t>(geometry->vertices.size()));
            geometry->vertices.push_back({attrib.vertices[mesh.indices[i].vertex_index * 3 + 0],
                                          attrib.vertices[mesh.indices[i].vertex_index * 3 + 1],
                                          attrib.vertices[mesh.indices[i].vertex_index * 3 + 2]});
            geometry->normals.push_back({attrib.normals[mesh.indices[i].normal_index * 3 + 0],
                                         attrib.normals[mesh.indices[i].normal_index * 3 + 1],
                                         attrib.normals[mesh.indices[i].normal_index * 3 + 2]});
            geometry->uvs.push_back({
                attrib.texcoords[mesh.indices[i].texcoord_index * 2 + 0],
                attrib.texcoords[mesh.indices[i].texcoord_index * 2 + 1],
            });
        }
        else
        {
            geometry->indices.push_back(static_cast<uint32_t>(it->second));
        }
    }
}

TextureSource *CreateTextureSource(std::string const &folder, std::string const &path)
{
    static std::unordered_map<std::string, TextureSource> s_textureSourceCache(11);

    std::string texturePath = folder + "/" + path;
    TextureSource *texture = &s_textureSourceCache[texturePath];
    if (texture->data != nullptr)
    {
        return texture;
    }

    texture->filepath = std::move(texturePath);
    texture->data = stbi_load(
        texture->filepath.c_str(), &texture->width, &texture->height, &texture->channels, 0);
    if (texture->data == nullptr)
    {
        std::cerr << "Failed to load texture: " << texture->filepath << std::endl;
    }

    switch (texture->channels)
    {
    case 1:
        texture->format = GL_RED;
        break;
    case 2:
        texture->format = GL_RG;
        break;
    case 3:
        texture->format = GL_RGB;
        break;
    case 4:
        texture->format = GL_RGBA;
        break;
    default:
        std::cerr << "Failed to detect texture format!" << std::endl;
        break;
    }

    return texture;
}

MaterialSource CreateMaterialSource(std::string const &folder, tinyobj::material_t const &material)
{
    TextureSource *albedo = nullptr;
    if (!material.diffuse_texname.empty())
    {
        albedo = CreateTextureSource(folder, material.diffuse_texname);
    }

    TextureSource *normal = nullptr;
    if (!material.normal_texname.empty())
    {
        normal = CreateTextureSource(folder, material.normal_texname);
    }

    TextureSource *bump = nullptr;
    if (!material.bump_texname.empty())
    {
        bump = CreateTextureSource(folder, material.bump_texname);
    }

    TextureSource *metallic = nullptr;
    if (!material.metallic_texname.empty())
    {
        metallic = CreateTextureSource(folder, material.metallic_texname);
    }

    TextureSource *roughness = nullptr;
    if (!material.roughness_texname.empty())
    {
        roughness = CreateTextureSource(folder, material.roughness_texname);
    }

    return MaterialSource{albedo, normal, bump, metallic, roughness};
}

void FreeMaterialSource(MaterialSource &material)
{
    if (material.albedo != nullptr && material.albedo->data != nullptr)
    {
        free(material.albedo->data);
        material.albedo->data = nullptr;
    }
    if (material.normal != nullptr && material.normal->data != nullptr)
    {
        free(material.normal->data);
        material.normal->data = nullptr;
    }
    if (material.bump != nullptr && material.bump->data != nullptr)
    {
        free(material.bump->data);
        material.bump->data = nullptr;
    }
    if (material.metallic != nullptr && material.metallic->data != nullptr)
    {
        free(material.metallic->data);
        material.metallic->data = nullptr;
    }
    if (material.roughness != nullptr && material.roughness->data != nullptr)
    {
        free(material.roughness->data);
        material.roughness->data = nullptr;
    }
}

bool LoadOBJ(std::string const &folder, std::string const &filename, std::vector<Geometry> &geometries, std::vector<MaterialSource> &materials)
{
    auto const filePath = std::string(folder) + "/" + filename;
    stbi_set_flip_vertically_on_load(true);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> rawGeometries;
    std::vector<tinyobj::material_t> rawMaterials;

    std::string warn;
    std::string err;

    bool const ret = tinyobj::LoadObj(&attrib, &rawGeometries, &rawMaterials, &warn, &err, filePath.c_str(), folder.c_str());

    if (!warn.empty())
    {
        std::cout << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cerr << err << std::endl;
    }

    if (!ret)
    {
        std::cerr << "Failed to load obj: " << filePath << std::endl;
        return false;
    }

    for (auto const &rawGeometry : rawGeometries)
    {
        LoadGeometry(attrib, rawGeometry, geometries);
    }

    materials.reserve(rawMaterials.size());
    for (auto const &material : rawMaterials)
    {
        materials.push_back(CreateMaterialSource(folder, material));
    }

    return true;
}

std::string LoadFile(char const *filepath)
{
    std::ifstream t(filepath);
    if (!t)
    {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return {};
    }

    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);

    return buffer;
}

std::vector<BufferDescriptor> CreateBufferDescriptors(Geometry &model)
{
    return std::vector<BufferDescriptor>{
        BufferDescriptor{sizeof(sr::math::Vec3), static_cast<uint32_t>(model.vertices.size()), model.vertices.data()},
        BufferDescriptor{sizeof(sr::math::Vec3), static_cast<uint32_t>(model.normals.size()), model.normals.data()},
        BufferDescriptor{sizeof(sr::math::Vec2), static_cast<uint32_t>(model.uvs.size()), model.uvs.data()}};
}

BufferDescriptor CreateIndexBufferDescriptor(Geometry &model)
{
    return BufferDescriptor{
        sizeof(uint32_t), static_cast<uint32_t>(model.indices.size()), model.indices.data()};
}
} // namespace sr::load