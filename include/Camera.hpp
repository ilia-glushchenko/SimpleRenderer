/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "RenderDefinitions.hpp"
#include "Math.hpp"

Camera CreateCamera()
{
    Camera camera;

    camera.fov = 1.0472f;
    camera.aspect = 1;
    camera.near = 0.1f;
    camera.far = 10000.f;
    camera.proj = sr::math::CreatePerspectiveProjectionMatrix(camera.near, camera.far, camera.fov, 1.f);
    camera.view = sr::math::CreateIdentityMatrix();
    //camera.pos = {-1217.f, 101.f, 40.f};
    camera.pos = {-1217.f, -2.3f, 41.f};
    camera.yWorldAndle = -1.46f;

    return camera;
}