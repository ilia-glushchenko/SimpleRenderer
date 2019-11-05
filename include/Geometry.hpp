/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <Math.hpp>
#include <cassert>
#include <limits>

namespace sr::geo
{

struct AABB
{
    sr::math::Vec3 min = {};
    sr::math::Vec3 max = {};
};

inline sr::math::Vec3
CalculateCenterOfMass(sr::math::Vec3 const *data, uint32_t const *indices, uint64_t length)
{
    assert(data != nullptr);
    assert(indices != nullptr);
    assert(length > 0);

    sr::math::Vec3 result = {};

    for (uint64_t i = 0; i < length; ++i)
    {
        result += data[indices[i]];
    }

    return result / static_cast<float>(length);
}

inline AABB CalculateAABB(sr::math::Vec3 const *data, uint64_t length)
{
    assert(data != nullptr);
    assert(length > 1);
    assert(data[0] != data[1]);

    AABB result = {
        {-FLT_MAX, -FLT_MAX, -FLT_MAX},
        {FLT_MAX, FLT_MAX, FLT_MAX}};

    for (uint64_t i = 0; i < length; ++i)
    {
        result.min.x = result.min.x > data[i].x ? data[i].x : result.min.x;
        result.min.y = result.min.y > data[i].y ? data[i].y : result.min.y;
        result.min.z = result.min.z > data[i].z ? data[i].z : result.min.z;

        result.max.x = result.max.x < data[i].x ? data[i].x : result.max.x;
        result.max.y = result.max.y < data[i].y ? data[i].y : result.max.y;
        result.max.z = result.max.z < data[i].z ? data[i].z : result.max.z;
    }

    return result;
}

inline AABB CalculateAABB(
    sr::math::Vec3 const *data, uint64_t length, sr::math::Vec3 center, sr::math::Matrix4x4 model)
{
    assert(data != nullptr);
    assert(length > 1);
    assert(data[0] != data[1]);
    assert(!IsNullMatrix(model));

    AABB result = {
        {FLT_MAX, FLT_MAX, FLT_MAX},
        {-FLT_MAX, -FLT_MAX, -FLT_MAX}};

    sr::math::Vec4 vertex;
    for (uint64_t i = 0; i < length; ++i)
    {
        vertex = model * sr::math::Vec4{data[i].x, data[i].y, data[i].z, 1};

        result.min.x = vertex.x < result.min.x ? vertex.x : result.min.x;
        result.min.y = vertex.y < result.min.y ? vertex.y : result.min.y;
        result.min.z = vertex.z < result.min.z ? vertex.z : result.min.z;

        result.max.x = vertex.x > result.max.x ? vertex.x : result.max.x;
        result.max.y = vertex.y > result.max.y ? vertex.y : result.max.y;
        result.max.z = vertex.z > result.max.z ? vertex.z : result.max.z;
    }

    result.min -= center;
    result.max -= center;

    return result;
}

} // namespace sr::geo
