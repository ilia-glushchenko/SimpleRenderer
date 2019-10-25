/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "Loader.hpp"

sr::load::Geometry g_triangle =
    {
        {{0.0f, 0.5f, 0},
         {0.5f, -0.5f, 0},
         {-0.5f, -0.5f, 0}},

        {{0, 0, 1},
         {0, 0, 1},
         {0, 0, 1}},

        {{0, 0},
         {0, 1},
         {0.5f, 1}},

        {0, 1, 2}};

sr::load::Geometry g_quadGround =
    {
        {
            {-0.5f, -0.25f, -0.5f},
            {0.5f, -0.25f, -0.5f},
            {0.5f, -0.25f, 0.5f},
            {-0.5f, -0.25f, 0.5f},
        },

        {
            {0, 1, 0},
            {0, 1, 0},
            {0, 1, 0},
            {0, 1, 0},
        },

        {
            {0, 0},
            {1, 0},
            {1, 1},
            {0, 1},
        },

        {0, 1, 2,
         0, 2, 3}};

sr::load::Geometry g_quadWall =
    {
        {
            {
                -1.0f,
                -1.0f,
                -0.0,
            },
            {
                1.0f,
                -1.0f,
                -0.0,
            },
            {
                1.0f,
                1.0f,
                -0.0,
            },
            {
                -1.0f,
                1.0f,
                -0.0,
            },
        },

        {
            {0, 0, 1},
            {0, 0, 1},
            {0, 0, 1},
            {0, 0, 1},
        },

        {
            {0, 0},
            {1, 0},
            {1, 1},
            {0, 1},
        },

        {0, 1, 2,
         0, 2, 3}};

static std::vector<AttributeDescriptor> const g_shaderAttributesPosition = {
    {"aPosition", 3, sizeof(sr::math::Vec3)},
};

static std::vector<AttributeDescriptor> const g_shaderAttributesPositionNormal = {
    {"aPosition", 3, sizeof(sr::math::Vec3)},
    {"aNormal", 3, sizeof(sr::math::Vec3)},
};

static std::vector<AttributeDescriptor> const g_shaderAttributesPositionUV = {
    {"aPosition", 3, sizeof(sr::math::Vec3)},
    {"aUV", 2, sizeof(sr::math::Vec2)},
};

static std::vector<AttributeDescriptor> const g_shaderAttributesPositionNormalUV = {
    {"aPosition", 3, sizeof(sr::math::Vec3)},
    {"aNormal", 3, sizeof(sr::math::Vec3)},
    {"aUV", 2, sizeof(sr::math::Vec2)},
};

static RenderModel g_quadWallRenderModel;

static RenderModel g_boxRenderModel;

constexpr sr::math::Vec2 g_taaHalton23Sequence8[8] = {
    sr::math::Vec2{0.5f, 0.33333333f},
    sr::math::Vec2{0.25f, 0.66666667f},
    sr::math::Vec2{0.75f, 0.11111111f},
    sr::math::Vec2{0.125f, 0.44444444f},
    sr::math::Vec2{0.625f, 0.77777778f},
    sr::math::Vec2{0.375f, 0.22222222f},
    sr::math::Vec2{0.875f, 0.55555556f},
    sr::math::Vec2{0.0625f, 0.88888889f},
};

constexpr sr::math::Vec2 g_taaHalton23Sequence16[16] = {
    sr::math::Vec2{0.5f, 0.33333333f},
    sr::math::Vec2{0.25f, 0.66666667f},
    sr::math::Vec2{0.75f, 0.11111111f},
    sr::math::Vec2{0.125f, 0.44444444f},
    sr::math::Vec2{0.625f, 0.77777778f},
    sr::math::Vec2{0.375f, 0.22222222f},
    sr::math::Vec2{0.875f, 0.55555556f},
    sr::math::Vec2{0.0625f, 0.88888889f},
    sr::math::Vec2{0.5625f, 0.03703704f},
    sr::math::Vec2{0.3125f, 0.37037037f},
    sr::math::Vec2{0.8125f, 0.7037037f},
    sr::math::Vec2{0.1875f, 0.14814815f},
    sr::math::Vec2{0.6875f, 0.48148148f},
    sr::math::Vec2{0.4375f, 0.81481481f},
    sr::math::Vec2{0.9375f, 0.25925926f},
    sr::math::Vec2{0.03125f, 0.5925925f},
};