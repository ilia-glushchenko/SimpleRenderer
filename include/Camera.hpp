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
    camera.pos = {-530.f, 150.f, -35.f};
    camera.xWorldAngle = 0.1f;
    camera.xWorldAngle = 0;
    camera.yWorldAngle = -1.57f;

    return camera;
}