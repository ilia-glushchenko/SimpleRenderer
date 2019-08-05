/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "Loader.hpp"

sr::load::Geometry g_triangle =
{
    {
        { 0.0f,  0.5f, 0 },
        { 0.5f, -0.5f, 0 },
        {-0.5f, -0.5f, 0 }
    },

    {
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1}
    },

    {
        {0, 0},
        {0, 1},
        {0.5f, 1}
    },

    {
        0, 1, 2
    }
};

sr::load::Geometry g_quadGround =
{
    {
        { -0.5f, -0.25f, -0.5f },
        {  0.5f, -0.25f, -0.5f },
        {  0.5f, -0.25f,  0.5f },
        { -0.5f, -0.25f,  0.5f },
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

    {
        0, 1, 2,
        0, 2, 3
    }
};

sr::load::Geometry g_quadWall =
{
    {
        { -1.0f, -1.0f, -0.0, },
        {  1.0f, -1.0f, -0.0, },
        {  1.0f,  1.0f, -0.0, },
        { -1.0f,  1.0f, -0.0, },
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

    {
        0, 1, 2,
        0, 2, 3
    }
};