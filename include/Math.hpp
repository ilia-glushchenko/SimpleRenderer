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

    inline constexpr Vec2 operator-() const noexcept
    {
        return {-x, -y};
    }

    inline constexpr Vec2 operator-(float s) const noexcept
    {
        return {x - s, y - s};
    }

    inline constexpr Vec2 operator*(float s) const noexcept
    {
        return {x * s, y * s};
    }

    inline constexpr Vec2 operator/(float s) const noexcept
    {
        return {x / s, y / s};
    }

    inline constexpr Vec2 &operator*=(float s) noexcept
    {
        return *this = *this * s;
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

    inline constexpr Vec3 &operator-=(Vec3 const &other) noexcept
    {
        return *this = *this - other;
    }

    inline constexpr Vec3 &operator+=(Vec3 const &other) noexcept
    {
        return *this = *this + other;
    }

    inline constexpr Vec3 operator-(Vec3 other) const noexcept
    {
        other.x = x - other.x;
        other.y = y - other.y;
        other.z = z - other.z;

        return other;
    }

    inline constexpr Vec3 operator+(Vec3 other) const noexcept
    {
        other.x += x;
        other.y += y;
        other.z += z;

        return other;
    }

    inline constexpr Vec3 operator-() const noexcept
    {
        return {-x, -y, -z};
    }

    inline constexpr Vec3 operator*(float s) const noexcept
    {
        return {x * s, y * s, z * s};
    }

    inline constexpr Vec3 operator/(float s) const noexcept
    {
        return {x / s, y / s, z / s};
    }

    inline constexpr bool operator==(Vec3 const &other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    inline constexpr bool operator!=(Vec3 const &other) const noexcept
    {
        return !(*this == other);
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
        Vec2 xy;
        Vec3 xyz;
        float data[4];
    };

    inline constexpr Vec4 &operator*=(float s) noexcept
    {
        return *this = *this * s;
    }

    inline constexpr Vec4 operator-() const noexcept
    {
        return {-x, -y, -z, -w};
    }

    inline constexpr Vec4 operator-(Vec4 other) const noexcept
    {
        other.x = x - other.x;
        other.y = y - other.y;
        other.z = z - other.z;
        other.w = w - other.w;

        return other;
    }

    inline constexpr Vec4 operator+(Vec4 other) const noexcept
    {
        other.x = x + other.x;
        other.y = y + other.y;
        other.z = z + other.z;
        other.w = w + other.w;

        return other;
    }

    inline constexpr Vec4 operator*(float s) const noexcept
    {
        return {x * s, y * s, z * s, w * s};
    }

    inline constexpr Vec4 operator/(float s) const noexcept
    {
        return {x / s, y / s, z / s, w / s};
    }

    inline constexpr bool operator==(Vec4 const &other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    inline constexpr bool operator!=(Vec4 const &other) const noexcept
    {
        return !(*this == other);
    }
};

struct Matrix4x4;
inline constexpr Matrix4x4 Mul(Matrix4x4 mat, float s) noexcept;
inline constexpr Vec4 Mul(Matrix4x4 mat, Vec4 vec) noexcept;
inline constexpr Matrix4x4 Mul(Matrix4x4 mat, Matrix4x4 vec) noexcept;

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

    inline constexpr Matrix4x4 operator*(float s) const noexcept
    {
        return Mul(*this, s);
    }

    inline constexpr Vec4 operator*(Vec4 v) const noexcept
    {
        return Mul(*this, v);
    }

    inline constexpr Matrix4x4 operator*(Matrix4x4 const &other) const noexcept
    {
        return Mul(*this, other);
    }
};

inline constexpr float Dot(Vec3 a, Vec3 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline constexpr float Dot(Vec4 a, Vec4 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline constexpr Matrix4x4 Mul(Matrix4x4 mat, float s) noexcept
{
    return {
        mat._1 * s,
        mat._2 * s,
        mat._3 * s,
        mat._4 * s};
}

inline constexpr Vec4 Mul(Matrix4x4 mat, Vec4 vec) noexcept
{
    Vec4 result = {};

    result.x = Dot(mat._1, vec);
    result.y = Dot(mat._2, vec);
    result.z = Dot(mat._3, vec);
    result.w = Dot(mat._4, vec);

    return result;
}

inline constexpr Matrix4x4 Mul(Matrix4x4 a, Matrix4x4 b) noexcept
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

inline constexpr Matrix4x4 Transpose(Matrix4x4 m) noexcept
{
    return {
        m._11, m._21, m._31, m._41,
        m._12, m._22, m._32, m._42,
        m._13, m._23, m._33, m._43,
        m._14, m._24, m._34, m._44};
}

inline constexpr Matrix4x4 CreateIdentityMatrix() noexcept
{
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
}

inline constexpr Matrix4x4 CreateOrthographicProjectionMatrix(float left, float right, float bottom, float top, float near, float far) noexcept
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

inline Matrix4x4 CreatePerspectiveProjectionMatrix(float near, float far, float fov, float aspect) noexcept
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

inline Matrix4x4 CreatePerspectiveProjectionMatrixSheared(
    float near, float far, float fov, float aspect, float xShear, float yShear) noexcept
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

inline constexpr Matrix4x4 CreatePerspectiveProjectionMatrix(
    float left, float right, float bottom, float top, float near, float far) noexcept
{
    Matrix4x4 mat = {};

    mat._11 = (2 * near) / (right - left);
    mat._22 = (2 * near) / (top - bottom);

    mat._13 = -(right + left) / (right - left);
    mat._23 = -(top + bottom) / (top - bottom);

    mat._33 = (far + near) / (far - near);
    mat._34 = -(2 * far * near) / (far - near);

    mat._43 = 1;

    return mat;
}

inline constexpr Matrix4x4 CreateChangeOfBasisMatrix(
    math::Vec3 dir, math::Vec3 up, math::Vec3 right, math::Vec3 pos) noexcept
{
    return {
        right.x, right.y, right.z, math::Dot(-right, pos),
        up.x, up.y, up.z, math::Dot(-up, pos),
        dir.x, dir.y, dir.z, math::Dot(-dir, pos),
        0, 0, 0, 1};
}

inline constexpr Matrix4x4 CreateScaleMatrix(float x, float y, float z) noexcept
{
    return {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1};
}

inline constexpr Matrix4x4 CreateScaleMatrix(float s) noexcept
{
    return CreateScaleMatrix(s, s, s);
}

inline constexpr Matrix4x4 CreateScaleMatrix(Vec3 v) noexcept
{
    return CreateScaleMatrix(v.x, v.y, v.z);
}

inline Matrix4x4 CreateRotationMatrixX(float a) noexcept
{
    auto const c = std::cos(a);
    auto const s = std::sin(a);

    return {
        1, 0, 0, 0,
        0, c, -s, 0,
        0, s, c, 0,
        0, 0, 0, 1};
}

inline Matrix4x4 CreateRotationMatrixY(float a) noexcept
{
    auto const c = std::cos(a);
    auto const s = std::sin(a);

    return {
        c, 0, s, 0,
        0, 1, 0, 0,
        -s, 0, c, 0,
        0, 0, 0, 1};
}

inline Matrix4x4 CreateRotationMatrixZ(float a) noexcept
{
    auto const c = std::cos(a);
    auto const s = std::sin(a);

    return {
        c, -s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
}

inline constexpr Matrix4x4 CreateTranslationMatrix(float x, float y, float z) noexcept
{
    return {
        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1};
}

inline constexpr Matrix4x4 CreateTranslationMatrix(Vec3 vec) noexcept
{
    return CreateTranslationMatrix(vec.x, vec.y, vec.z);
}

inline float CreateUniformRandomFloat(float min, float max) noexcept
{
    return (std::rand() % static_cast<int32_t>(max - min)) - min;
}

inline Vec3 CreateUniformRandomVec3(float min, float max) noexcept
{
    return {
        CreateUniformRandomFloat(min, max),
        CreateUniformRandomFloat(min, max),
        CreateUniformRandomFloat(min, max),
    };
}

inline constexpr bool IsNullMatrix(Matrix4x4 const &m) noexcept
{
    return m._11 == 0 && m._21 == 0 && m._31 == 0 && m._41 == 0 &&
           m._12 == 0 && m._22 == 0 && m._32 == 0 && m._42 == 0 &&
           m._13 == 0 && m._23 == 0 && m._33 == 0 && m._43 == 0 &&
           m._14 == 0 && m._24 == 0 && m._34 == 0 && m._44 == 0;
}

} // namespace sr::math
