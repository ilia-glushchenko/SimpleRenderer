/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <cmath>
#include <cstdint>

namespace sr::math
{
struct Vec2
{
    union {
        struct
        {
            float x, y;
        };
        struct
        {
            float u, v;
        };
        struct
        {
            float r, g;
        };
        float data[2];
    };

    constexpr Vec2 operator-() const
    {
        return {-x, -y};
    }

    constexpr Vec2 operator/(float s) const
    {
        return {x / s, y / s};
    }
};

struct Vec3
{
    union {
        struct
        {
            float x, y, z;
        };
        struct
        {
            float r, g, b;
        };
        float data[3];
    };

    constexpr Vec3 operator+(Vec3 const &other) const
    {
        return {other.x + x, other.y + y, other.z + z};
    }

    constexpr Vec3 operator-() const
    {
        return {-x, -y, -z};
    }
};

struct Vec4
{
    union {
        struct
        {
            float x, y, z, w;
        };
        struct
        {
            float r, g, b, a;
        };
        float data[4];
    };

    constexpr Vec4 operator-() const
    {
        return {-x, -y, -z, -w};
    }

    constexpr Vec4 operator*(float s) const
    {
        return {x * s, y * s, z * s, w * s};
    }

    constexpr Vec4 operator/(float s) const
    {
        return {x / s, y / s, z / s, w / s};
    }
};

struct Matrix4x4
{
    union {
        struct
        {
            Vec4 _1, _2, _3, _4;
        };
        struct
        {
            float _11, _12, _13, _14,
                _21, _22, _23, _24,
                _31, _32, _33, _34,
                _41, _42, _43, _44;
        };
        float data[16];
        float rows[4][4];
    };
};

constexpr float Dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr float Dot(Vec4 a, Vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

constexpr Matrix4x4 Mul(Matrix4x4 mat, float s)
{
    return {
        mat._1 * s,
        mat._2 * s,
        mat._3 * s,
        mat._4 * s};
}

constexpr Vec4 Mul(Matrix4x4 mat, Vec4 vec)
{
    Vec4 result = {};

    result.x = Dot(mat._1, vec);
    result.y = Dot(mat._2, vec);
    result.z = Dot(mat._3, vec);
    result.w = Dot(mat._4, vec);

    return result;
}

constexpr Matrix4x4 Mul(Matrix4x4 a, Matrix4x4 b)
{
    Matrix4x4 result = {};

    for (uint8_t i = 0; i < 4; ++i)
    {
        for (uint8_t j = 0; j < 4; ++j)
        {
            for (uint8_t k = 0; k < 4; ++k)
            {
                result.rows[i][j] += a.rows[i][k] * b.rows[k][j];
            }
        }
    }

    return result;
}

constexpr Matrix4x4 Transpose(Matrix4x4 m)
{
    return {
        m._11, m._21, m._31, m._41,
        m._12, m._22, m._32, m._42,
        m._13, m._23, m._33, m._43,
        m._14, m._24, m._34, m._44};
}

constexpr Matrix4x4 CreateIdentityMatrix()
{
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
}

constexpr Matrix4x4 CreateOrthographicProjectionMatrix(float left, float right, float bottom, float top, float near, float far)
{
    Matrix4x4 mat = {};

    mat._11 = 2.f / (right - left);
    mat._14 = -(right + left) / (right - left);

    mat._22 = 2.f / (top - bottom);
    mat._24 = -(top + bottom) / (top - bottom);

    mat._33 = 2.f / (far - near);
    mat._34 = -(far + near) / (far - near);

    mat._44 = 1.f;

    return mat;
}

Matrix4x4 CreatePerspectiveProjectionMatrix(float near, float far, float fov, float aspect)
{
    Matrix4x4 mat = {};

    float const c = 1.f / std::tan(fov * 0.5f);

    mat._11 = c / aspect;

    mat._22 = c;

    mat._33 = -(far + near) / (far - near);
    mat._34 = -(2.f * far * near) / (far - near);

    mat._43 = -1.f;

    return mat;
}

Matrix4x4 CreatePerspectiveProjectionMatrixSheared(float near, float far, float fov, float aspect, float xShear, float yShear)
{
    Matrix4x4 mat = CreatePerspectiveProjectionMatrix(near, far, fov, aspect);

    float a = 2 * xShear;
    float b = aspect * std::tan(fov * 0.5f) * 2.f * near;
    float left = (a - b) / 2.f;
    float right = a - left;
    mat._13 = -(right + left) / (right - left);

    a = 2 * yShear;
    b = 2 * near * std::tan(fov * 0.5f);
    float top = (a - b) / 2.f;
    float bottom = a - top;
    mat._23 = -(top + bottom) / (top - bottom);

    return mat;
}

constexpr Matrix4x4 CreatePerspectiveProjectionMatrix(float left, float right, float bottom, float top, float near, float far)
{
    Matrix4x4 mat = {};

    mat._11 = (2 * near) / (right - left);
    mat._13 = -(right + left) / (right - left);

    mat._22 = (2 * near) / (top - bottom);
    mat._23 = -(top + bottom) / (top - bottom);

    mat._33 = (far + near) / (far - near);
    mat._34 = -(2 * far * near) / (far - near);

    mat._43 = 1;

    return mat;
}

constexpr Matrix4x4 CreateChangeOfBasisMatrix(math::Vec3 dir, math::Vec3 up, math::Vec3 right, math::Vec3 pos)
{
    return {
        right.x, right.y, right.z, math::Dot(-right, pos),
        up.x, up.y, up.z, math::Dot(-up, pos),
        dir.x, dir.y, dir.z, math::Dot(-dir, pos),
        0, 0, 0, 1};
}

Matrix4x4 CreateScaleMatrix(float x, float y, float z)
{
    return {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1};
}

Matrix4x4 CreateScaleMatrix(float s)
{
    return CreateScaleMatrix(s, s, s);
}

Matrix4x4 CreateRotationMatrixX(float a)
{
    auto const c = std::cos(a);
    auto const s = std::sin(a);

    return {
        1, 0, 0, 0,
        0, c, -s, 0,
        0, s, c, 0,
        0, 0, 0, 1};
}

Matrix4x4 CreateRotationMatrixY(float a)
{
    auto const c = std::cos(a);
    auto const s = std::sin(a);

    return {
        c, 0, s, 0,
        0, 1, 0, 0,
        -s, 0, c, 0,
        0, 0, 0, 1};
}

Matrix4x4 CreateRotationMatrixZ(float a)
{
    auto const c = std::cos(a);
    auto const s = std::sin(a);

    return {
        c, -s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
}

Matrix4x4 CreateTranslationMatrix(float x, float y, float z)
{
    return {
        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1};
}

float CreateUniformRandomFloat(float min, float max)
{
    return (std::rand() % static_cast<int32_t>(max - min)) - min;
}

Vec3 CreateUniformRandomVec3(float min, float max)
{
    return {
        CreateUniformRandomFloat(min, max),
        CreateUniformRandomFloat(min, max),
        CreateUniformRandomFloat(min, max),
    };
}

} // namespace sr::math
