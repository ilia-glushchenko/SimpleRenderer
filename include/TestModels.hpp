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

constexpr uint32_t g_taaSubPixelSampleCount = 16;
const sr::math::Vec2 g_taaSubPixelSamples[16] = {
    sr::math::Vec2{-8.0f, 0.0f} / 8.0f,
    sr::math::Vec2{-6.0f, -4.0f} / 8.0f,
    sr::math::Vec2{-3.0f, -2.0f} / 8.0f,
    sr::math::Vec2{-2.0f, -6.0f} / 8.0f,
    sr::math::Vec2{1.0f, -1.0f} / 8.0f,
    sr::math::Vec2{2.0f, -5.0f} / 8.0f,
    sr::math::Vec2{6.0f, -7.0f} / 8.0f,
    sr::math::Vec2{5.0f, -3.0f} / 8.0f,
    sr::math::Vec2{4.0f, 1.0f} / 8.0f,
    sr::math::Vec2{7.0f, 4.0f} / 8.0f,
    sr::math::Vec2{3.0f, 5.0f} / 8.0f,
    sr::math::Vec2{0.0f, 7.0f} / 8.0f,
    sr::math::Vec2{-1.0f, 3.0f} / 8.0f,
    sr::math::Vec2{-4.0f, 6.0f} / 8.0f,
    sr::math::Vec2{-7.0f, 8.0f} / 8.0f,
    sr::math::Vec2{-5.0f, 2.0f} / 8.0f};