/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "RenderDefinitions.hpp"
#include "RenderModel.hpp"
#include "TestModels.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace
{

void GLAPIENTRY
MessageCallback(GLenum source,
                GLenum type,
                GLuint id,
                GLenum severity,
                GLsizei length,
                const GLchar *message,
                const void *userParam)
{
    if (type != GL_DEBUG_TYPE_OTHER && type != GL_DEBUG_TYPE_MARKER)
    {
        fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n\n",
                (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
                type, severity, message);
    }
}

} // namespace

GLFWwindow *InitializeGLFW(uint32_t width, uint32_t height)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

    GLFWwindow *window = glfwCreateWindow(width, height, "Simple Renderer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glbinding::Binding::initialize(glfwGetProcAddress);

    return window;
}

void InitializeImGui(GLFWwindow *window)
{
    const char *glsl_version = "#version 460";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    auto &io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
}

void ConfigureGL()
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(::MessageCallback, 0);

    glClearColor(1, 1, 1, 1);
    glClearDepth(1.0f);

    glEnable(GL_DEPTH_TEST);

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
}

void InitializeGlobals()
{
    auto const vertexBufferDescriptors = sr::load::CreateBufferDescriptors(g_quadWall);
    auto const indexBufferDescriptor = sr::load::CreateIndexBufferDescriptor(g_quadWall);
    sr::load::MaterialSource const emptyMaterial = {};

    RenderModelCreateInfo createInfo;
    createInfo.color = {};
    createInfo.debugRenderModel = 0;
    createInfo.geometry = &g_quadWall;
    createInfo.indexBufferDescriptor = &indexBufferDescriptor;
    createInfo.vertexBufferDescriptors = &vertexBufferDescriptors;
    createInfo.model = sr::math::CreateIdentityMatrix();
    createInfo.material = &emptyMaterial;

    g_quadWallRenderModel = CreateRenderModel(createInfo);
}

void DeinitializeImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DeinitializeGLFW(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
