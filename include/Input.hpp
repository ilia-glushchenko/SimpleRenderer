/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <RenderConfiguration.hpp>

namespace sr::input
{

enum class Keys : uint32_t
{
    A = 65,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    F1 = 290,
    F2,
    F3,
    F4,
    F5,
    F6,

    Count
};

enum class KeyAction : uint8_t
{
    NONE = 0,
    PRESS,
    REPEAT,
    RELEASE,

    Count
};

struct Inputs
{
    KeyAction keys[static_cast<uint32_t>(Keys::Count)] = {};
} g_inputs;

void UpdateInputs()
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(Keys::Count); ++i)
    {
        if (g_inputs.keys[i] == KeyAction::RELEASE)
        {
            g_inputs.keys[i] = KeyAction::NONE;
        }
    }

    glfwPollEvents();
}

void ProcessInput(Keys key, KeyAction action)
{
    if (g_inputs.keys[static_cast<uint32_t>(key)] == KeyAction::NONE &&
        action == KeyAction::PRESS)
    {
        g_inputs.keys[static_cast<uint32_t>(key)] = action;
    }
    else if (g_inputs.keys[static_cast<uint32_t>(key)] == KeyAction::PRESS &&
             action == KeyAction::REPEAT)
    {
        g_inputs.keys[static_cast<uint32_t>(key)] = action;
    }
    else if (g_inputs.keys[static_cast<uint32_t>(key)] == KeyAction::REPEAT &&
             action == KeyAction::REPEAT)
    {
        g_inputs.keys[static_cast<uint32_t>(key)] = action;
    }
    else if ((g_inputs.keys[static_cast<uint32_t>(key)] == KeyAction::REPEAT ||
              g_inputs.keys[static_cast<uint32_t>(key)] == KeyAction::PRESS) &&
             action == KeyAction::NONE)
    {
        g_inputs.keys[static_cast<uint32_t>(key)] = KeyAction::RELEASE;
    }
}

void GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto &io = ImGui::GetIO();

    switch (key)
    {
    case GLFW_KEY_A:
    case GLFW_KEY_D:
    case GLFW_KEY_E:
    case GLFW_KEY_F:
    case GLFW_KEY_S:
    case GLFW_KEY_Q:
    case GLFW_KEY_W:
    case GLFW_KEY_U:
    case GLFW_KEY_F3:
    case GLFW_KEY_F4:
    case GLFW_KEY_F5:
        ProcessInput(static_cast<Keys>(key), static_cast<KeyAction>(action));
        break;
    default:
        io.KeysDown[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
        break;
    }
}

} // namespace sr::input