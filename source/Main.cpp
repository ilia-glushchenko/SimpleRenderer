/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#include "Math.hpp"
#include "Loader.hpp"
#include "Renderer.hpp"
#include "RenderPass.hpp"
#include "TestModels.hpp"

#include <ctime>

Camera g_camera = CreateCamera();
DirectionalLightSource g_directLight = {
    sr::math::CreateOrthographicProjectionMatrix(-2048.f, 2048.f, -2048.f, 2048.f, 1.f, 2000.f),
    sr::math::CreateChangeOfBasisMatrix({0.f, -1.f, 0.f}, {0.f, 0.f, -1.f}, {1.f, 0.f, 0.f}, {0.f, 1000.f, 0.f}),
};
constexpr uint32_t g_defaultWidth = 800;
constexpr uint32_t g_defaultHeight = 800;
TAABuffer g_taaBuffer = {};

bool g_captureMouse = false;
bool g_drawUi = false;
bool g_isHotRealoadRequired = false;
bool g_isPipelineReloadRequired = false;
uint32_t g_shadowMapsMode = 1;
uint32_t g_bumpMappingEnabled = 1;
float g_bumpMapScaleFactor = 0.00001f;
uint32_t g_bumpMapAvailable = 0;
uint32_t g_metallicMapAvailable = 0;
uint32_t g_roughnessMapAvailable = 0;
float g_lightPosition[3] = {0, 100, -45};
char const *g_disableEnableList[2] = {"Disabled", "Enabled"};
enum class eRenderMode : uint32_t
{
    Full = 0,
    Normal = 1,
    NormalMap = 2,
    BumpMap = 3,
    Depth = 4,
    ShadowMap = 5,
    MetallicMap = 6,
    Roughnessmap = 7,
    UVNNMap = 8,
    UVMipMap = 9,
    UVAnisMap = 10,
    Count
} g_renderMode = {};
char const *g_renderModesStr[static_cast<uint32_t>(eRenderMode::Count)] = {
    "Full",
    "Normal",
    "NormalMap",
    "BumpMap",
    "Depth",
    "ShadowMap",
    "MetallicMap",
    "RoughnessMap",
    "UVNNMap",
    "UVMipMap",
    "UVAnisMap"};

sr::math::Matrix4x4 CreateViewMatrix(sr::math::Vec3 pos, float yWorldAngle)
{
    sr::math::Matrix4x4 rotation = sr::math::CreateIdentityMatrix();

    rotation._11 = std::cos(yWorldAngle);
    rotation._13 = std::sin(yWorldAngle);
    rotation._31 = -std::sin(yWorldAngle);
    rotation._33 = std::cos(yWorldAngle);

    rotation = sr::math::Transpose(rotation);

    sr::math::Matrix4x4 translation = sr::math::CreateIdentityMatrix();
    translation._14 = -pos.x;
    translation._24 = -pos.y;
    translation._34 = -pos.z;

    return sr::math::Transpose(sr::math::Mul(rotation, translation));
}

void GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto &io = ImGui::GetIO();

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_W:
            g_camera.pos.z -= 5.05f;
            break;
        case GLFW_KEY_S:
            g_camera.pos.z += 5.05f;
            break;
        case GLFW_KEY_Q:
            g_camera.pos.y -= 5.05f;
            break;
        case GLFW_KEY_E:
            g_camera.pos.y += 5.05f;
            break;
        case GLFW_KEY_A:
            g_camera.pos.x -= 5.05f;
            break;
        case GLFW_KEY_D:
            g_camera.pos.x += 5.05f;
            break;
        default:
            io.KeysDown[key] = true;
            break;
        }
    }
    else if (action == GLFW_RELEASE)
    {
        switch (key)
        {
        case GLFW_KEY_F:
            g_captureMouse = !g_captureMouse;
            glfwSetInputMode(window, GLFW_CURSOR, g_captureMouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            break;
        case GLFW_KEY_F1:
            g_shadowMapsMode = g_shadowMapsMode + 1 == 2 ? 0 : 1;
            break;
        case GLFW_KEY_F2:
            g_bumpMappingEnabled = g_bumpMappingEnabled + 1 == 2 ? 0 : 1;
            break;
        case GLFW_KEY_F3:
            g_renderMode = g_renderMode == eRenderMode::Full ? g_renderMode = eRenderMode::Count : g_renderMode;
            g_renderMode = static_cast<eRenderMode>(static_cast<uint32_t>(g_renderMode) - 1);
            break;
        case GLFW_KEY_F4:
            g_renderMode = static_cast<eRenderMode>(static_cast<uint32_t>(g_renderMode) + 1);
            g_renderMode = g_renderMode == eRenderMode::Count ? g_renderMode = eRenderMode::Full : g_renderMode;
            break;
        case GLFW_KEY_F5:
            printf("\033c");
            g_isHotRealoadRequired = true;
            break;
        case GLFW_KEY_U:
            g_drawUi = !g_drawUi;
            break;
        default:
            io.KeysDown[key] = false;
            break;
        }
    }
}

void GLFWWindowSizeCallback(GLFWwindow *window, int width, int height)
{
    g_camera.fov = 1.0472f;
    g_camera.aspect = static_cast<float>(width) / height;

    g_camera.proj = sr::math::CreatePerspectiveProjectionMatrix(g_camera.near, g_camera.far, g_camera.fov, g_camera.aspect);
    g_taaBuffer.jitter = sr::math::CreateIdentityMatrix();

    g_isHotRealoadRequired = true;
    g_isPipelineReloadRequired = true;
}

void GLFWCursorPosCallback(GLFWwindow *window, double x, double y)
{
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
    {
        static double prevx = x, prevy = y;

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        double const normalizedDeltaX = (prevx - x) / width;
        double const normalizedDeltaY = (prevy - y) / height;

        float const yWorldAngle = static_cast<float>(normalizedDeltaX) * 3.14f;
        float const xWorldAngle = static_cast<float>(normalizedDeltaY) * 3.14f;

        g_camera.yWorldAndle = yWorldAngle;
    }
}

void SetupGLFWCallbacks(GLFWwindow *window)
{
    glfwSetKeyCallback(window, GLFWKeyCallback);
    glfwSetWindowSizeCallback(window, GLFWWindowSizeCallback);
    glfwSetCursorPosCallback(window, GLFWCursorPosCallback);
}

void DrawUI(GLFWwindow *window)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug window");
    ImGui::Text("Render mode: %s", g_renderModesStr[static_cast<uint32_t>(g_renderMode)]);
    ImGui::Combo("Shadows", reinterpret_cast<int *>(&g_shadowMapsMode), g_disableEnableList, IM_ARRAYSIZE(g_disableEnableList));
    ImGui::Combo("Bump Mapping", reinterpret_cast<int *>(&g_bumpMappingEnabled), g_disableEnableList, IM_ARRAYSIZE(g_disableEnableList));
    ImGui::SliderFloat("Bumpmap scale factor", &g_bumpMapScaleFactor, 0.0001f, 0.01f, "%.5f");
    ImGui::InputFloat3("Light pos:", g_lightPosition, 1);
    ImGui::InputFloat3("Camera pos:", g_camera.pos.data, 1);
    ImGui::InputFloat("Camera Y rad", &g_camera.yWorldAndle, 0.1f, 0.3f, 2);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::vector<RenderModel> LoadModels(GLuint program)
{
    std::vector<RenderModel> models;
    std::vector<sr::load::Geometry> geometries;
    std::vector<sr::load::MaterialSource> materials;

    //tinyobj::material_t material;
    //material.diffuse_texname = "sexkaitb_8K_Albedo.jpg";
    //material.normal_texname = "sexkaitb_8K_Normal.jpg";
    //material.bump_texname = "sexkaitb_8K_Bump.jpg";
    //material.roughness_texname = "sexkaitb_8K_Roughness.jpg";
    //sr::load::LoadOBJ("data\\quad", "quad.obj", geometries, materials);
    //models = { CreateRenderModel(attributes,
    //                      sr::load::CreateBufferDescriptors(geometries.back()),
    //                      sr::load::CreateIndexBufferDescriptor(geometries.back()),
    //                      sr::load::CreateMaterialSource("data\\materials\\Brickwall_Worn_sexkaitb_8K_surface_ms", material))};
    //g_camera.pos.z += 1;

    sr::load::LoadOBJ("data\\Sponza", "sponza.obj", geometries, materials);
    for (uint32_t i = 0; i < geometries.size(); ++i)
    {
        models.push_back(CreateRenderModel(sr::load::CreateBufferDescriptors(geometries[i]),
                                           sr::load::CreateIndexBufferDescriptor(geometries[i]),
                                           materials[geometries[i].material]));
        LinkRenderModelToShaderProgram(program, models.back(), g_defaultShaderProgramAttributes);
    }

    for (auto &material : materials)
    {
        sr::load::FreeMaterialSource(material);
    }

    return models;
}

RenderPass CreateRenderPassShadowMapping(GLFWwindow *window, ShaderProgram program)
{
    RenderPass pass;
    pass.program = program;
    glGenFramebuffers(1, &pass.fbo1);
    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo1);
    {
        pass.depthTexture = CreateDepthTexture(4096, 4096);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pass.depthTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass.depthTexture, 0);
        glDrawBuffers(1, &GL_NONE);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    { //Check for FBO completeness
        std::cerr << "Error! ShadowMapping FrameBuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return pass;
}

RenderPass CreateRenderPassShadowMapping(GLFWwindow *window)
{
    auto program = CreateShaderProgram(
        "shaders/shadow_mapping.vert", "shaders/shadow_mapping.frag",
        UniformsDescriptor{
            {}, {}, {}, UniformsDescriptor::MAT4{{"uProjMat", "uViewMat"}, {g_directLight.projection.data, g_directLight.view.data}}});

    return CreateRenderPassShadowMapping(window, program);
}

RenderPass CreateRenderPassLighting(GLFWwindow *window, int32_t width, int32_t height, ShaderProgram program)
{
    RenderPass pass;
    glGenFramebuffers(1, &pass.fbo1);
    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo1);
    {
        pass.program = program;
        pass.colorTexture = CreateTexture(
            sr::load::TextureSource{"", nullptr, width, height, 4, GL_RGBA});
        pass.depthTexture = CreateDepthTexture(width, height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pass.colorTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass.colorTexture, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pass.depthTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pass.depthTexture, 0);

        glDrawBuffers(1, &GL_COLOR_ATTACHMENT0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    { //Check for FBO completeness
        std::cerr << "Error! Lighting FrameBuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return pass;
}

RenderPass CreateRenderPassLighting(GLFWwindow *window, int32_t width, int32_t height)
{
    auto program = CreateShaderProgram(
        "shaders/lighting.vert", "shaders/lighting.frag",
        UniformsDescriptor{
            //uint32_t
            UniformsDescriptor::UINT32{
                {"uRenderModeUint",
                 "uShadowMappingEnabledUint",
                 "uBumpMappingEnabledUint",
                 "uBumpMapAvailableUint",
                 "uMetallicMapAvailableUint",
                 "uRoughnessMapAvailableUint"},
                {reinterpret_cast<uint32_t *>(&g_renderMode),
                 &g_shadowMapsMode,
                 &g_bumpMappingEnabled,
                 &g_bumpMapAvailable,
                 &g_metallicMapAvailable,
                 &g_roughnessMapAvailable}},
            //floats
            UniformsDescriptor::FLOAT{
                {"uBumpMapScaleFactorFloat"},
                {&g_bumpMapScaleFactor}},
            //float3
            UniformsDescriptor::FLOAT3{
                {"uCameraPos", "uPointLightPos"},
                {g_camera.pos.data, g_lightPosition}},
            //mat4
            UniformsDescriptor::MAT4{
                {"uProjMat",
                 "uViewMat",
                 "uJitterMat",
                 "uDirLightProjMat",
                 "uDirLightViewMat"},
                {g_camera.proj.data,
                 g_camera.view.data,
                 g_taaBuffer.jitter.data,
                 g_directLight.projection.data,
                 g_directLight.view.data}}});

    return CreateRenderPassLighting(window, width, height, program);
}

RenderPass CreateRenderPassTAA(GLFWwindow *window, ShaderProgram program)
{
    RenderPass pass;
    pass.program = program;
    glGenFramebuffers(2, pass.fbos);

    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo1);
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_taaBuffer.drawTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_taaBuffer.drawTexture, 0);
        glDrawBuffers(1, &GL_COLOR_ATTACHMENT0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error! TAA FrameBuffer 1 is not complete!" << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo2);
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_taaBuffer.historyTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_taaBuffer.historyTexture, 0);
        glDrawBuffers(1, &GL_COLOR_ATTACHMENT0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error! TAA FrameBuffer 2 is not complete!" << std::endl;
        }
    }

    return pass;
}

RenderPass CreateRenderPassTAA(GLFWwindow *window, int32_t width, int32_t height)
{
    DeleteTAABuffer(g_taaBuffer);
    g_taaBuffer = CreateTAABuffer(width, height);

    auto program = CreateShaderProgram(
        "shaders/taa.vert", "shaders/taa.frag",
        UniformsDescriptor{
            UniformsDescriptor::UINT32{
                {"uFrameCountUint"},
                {&g_taaBuffer.count}},
            UniformsDescriptor::FLOAT{
                {"uNearFloat", "uFarFloat"},
                {&g_camera.near, &g_camera.far}},
            UniformsDescriptor::FLOAT3{},
            UniformsDescriptor::MAT4{
                {"uViewMat", "uProjMat", "uJitterMat", "uPrevViewMat", "uPrevProjMat"},
                {g_camera.view.data,
                 g_camera.proj.data,
                 g_taaBuffer.jitter.data,
                 g_taaBuffer.prevView.data,
                 g_taaBuffer.prevProj.data}},
        });

    LinkRenderModelToShaderProgram(program.handle, g_quadWallRenderModel, g_defaultShaderProgramAttributes);

    return CreateRenderPassTAA(window, program);
}

RenderPass CreateRenderPassDebug(GLFWwindow *window, int32_t width, int32_t height, ShaderProgram program)
{
    RenderPass pass;
    pass.program = program;
    glGenFramebuffers(1, pass.fbos);

    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo1);
    {
        pass.program = program;
        pass.colorTexture = CreateTexture(
            sr::load::TextureSource{"", nullptr, width, height, 4, GL_RGBA});

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pass.colorTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass.colorTexture, 0);
        glDrawBuffers(1, &GL_COLOR_ATTACHMENT0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error! Debug FrameBuffer is not complete!" << std::endl;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return pass;
}

RenderPass CreateRenderPassDebug(GLFWwindow *window, int32_t width, int32_t height)
{
    auto program = CreateShaderProgram(
        "shaders/debug.vert", "shaders/debug.frag",
        UniformsDescriptor{
            UniformsDescriptor::UINT32{},
            UniformsDescriptor::FLOAT{},
            UniformsDescriptor::FLOAT3{},
            UniformsDescriptor::MAT4{},
        });

    LinkRenderModelToShaderProgram(program.handle, g_quadWallRenderModel, g_defaultShaderProgramAttributes);

    return CreateRenderPassDebug(window, width, height, program);
}

void RenderPassShadowMap(GLFWwindow *window, RenderPass const &pass, std::vector<RenderModel> const &models)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass.fbo1);

    glUseProgram(pass.program.handle);
    glViewport(0, 0, 4096, 4096);
    glScissor(0, 0, 4096, 4096);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_camera.view = CreateViewMatrix(g_camera.pos, g_camera.yWorldAndle);

    UpdateGlobalUniforms(pass.program);
    auto const modelMatrixLocation = glGetUniformLocation(pass.program.handle, "uModelMat");

    for (auto &model : models)
    {
        { // Per model uniforms update
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_TRUE, model.model.data);
        }

        glBindVertexArray(model.vertexArrayObject);
        glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void RenderPassLighting(GLFWwindow *window, RenderPass const &pass, std::vector<RenderModel> const &models)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo1);
    {
        g_camera.view = CreateViewMatrix(g_camera.pos, g_camera.yWorldAndle);
        g_camera.proj = sr::math::CreatePerspectiveProjectionMatrix(g_camera.near, g_camera.far, g_camera.fov, g_camera.aspect);
        const uint32_t taaSampleIndex = g_taaBuffer.count % g_taaSubPixelSampleCount;
        g_taaBuffer.jitter._14 = g_taaSubPixelSamples[taaSampleIndex].x / pass.width;
        g_taaBuffer.jitter._24 = g_taaSubPixelSamples[taaSampleIndex].y / pass.height;

        glUseProgram(pass.program.handle);
        glViewport(0, 0, pass.width, pass.height);
        glScissor(0, 0, pass.width, pass.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        BindRenderPassDependencies(pass.dependencies, pass.dependencyCount);
        UpdateGlobalUniforms(pass.program);
        auto const colorVectorLocation = glGetUniformLocation(pass.program.handle, "uColor");
        auto const modelMatrixLocation = glGetUniformLocation(pass.program.handle, "uModelMat");

        for (uint32_t i = 0; i < models.size(); ++i)
        {
            { // Per model uniforms update
                g_bumpMapAvailable = models[i].bumpTexture;
                g_metallicMapAvailable = models[i].metallicTexture;
                g_roughnessMapAvailable = models[i].roughnessTexture;

                glUniform3fv(colorVectorLocation, 1, models[i].color.data);
                glUniformMatrix4fv(modelMatrixLocation, 1, GL_TRUE, models[i].model.data);
            }

            BindDrawModelDependencies(models[i]);
            DrawModel(models[i]);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPassTAA(RenderPass &pass)
{
    ExecuteRenderPass(pass, &g_quadWallRenderModel, 1);

    std::swap(pass.fbo1, pass.fbo2);
    std::swap(g_taaBuffer.drawTexture, g_taaBuffer.historyTexture);

    g_taaBuffer.prevProj = g_camera.proj;
    g_taaBuffer.prevView = g_camera.view;
    g_taaBuffer.count++;
}

void RenderPassDebug(RenderPass const &pass)
{
    ExecuteRenderPass(pass, &g_quadWallRenderModel, 1);
}

void MainLoop(GLFWwindow *window)
{
    static int swapchainFramebufferWidth = 0, swapchainFramebufferHeight = 0;
    glfwGetFramebufferSize(window, &swapchainFramebufferWidth, &swapchainFramebufferHeight);

    auto shadowMappingPass = CreateRenderPassShadowMapping(window);
    auto lightingPass = CreateRenderPassLighting(window, swapchainFramebufferWidth, swapchainFramebufferHeight);
    auto taaPass = CreateRenderPassTAA(window, swapchainFramebufferWidth, swapchainFramebufferHeight);
    auto debugPass = CreateRenderPassDebug(window, swapchainFramebufferWidth, swapchainFramebufferHeight);
    auto models = LoadModels(lightingPass.program.handle);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glfwGetFramebufferSize(window, &swapchainFramebufferWidth, &swapchainFramebufferHeight);

        if (g_isHotRealoadRequired)
        {
            DeleteRenderPass(lightingPass);
            DeleteRenderPass(shadowMappingPass);
            DeleteRenderPass(taaPass);
            DeleteRenderPass(debugPass);

            shadowMappingPass = CreateRenderPassShadowMapping(window);
            lightingPass = CreateRenderPassLighting(window, swapchainFramebufferWidth, swapchainFramebufferHeight);
            taaPass = CreateRenderPassTAA(window, swapchainFramebufferWidth, swapchainFramebufferHeight);
            debugPass = CreateRenderPassDebug(window, swapchainFramebufferWidth, swapchainFramebufferHeight);

            std::time_t const timestamp = std::time(nullptr);
            std::cout << "Backbuffer size: " << swapchainFramebufferWidth << "x" << swapchainFramebufferHeight
                      << "\nHot reload: " << std::asctime(std::localtime(&timestamp)) << std::endl;

            g_isHotRealoadRequired = false;
            g_isPipelineReloadRequired = false;
        }
        else if (g_isPipelineReloadRequired)
        {
            lightingPass = CreateRenderPassLighting(window, swapchainFramebufferWidth, swapchainFramebufferHeight, lightingPass.program);
            shadowMappingPass = CreateRenderPassShadowMapping(window, shadowMappingPass.program);
            taaPass = CreateRenderPassTAA(window, taaPass.program);
            debugPass = CreateRenderPassDebug(window, swapchainFramebufferWidth, swapchainFramebufferHeight, debugPass.program);

            std::time_t const timestamp = std::time(nullptr);
            std::cout << "Backbuffer size: " << swapchainFramebufferWidth << "x" << swapchainFramebufferHeight
                      << "\nPipeline update: " << std::asctime(std::localtime(&timestamp)) << std::endl;

            g_isPipelineReloadRequired = false;
        }

        RenderPassShadowMap(window, shadowMappingPass, models);

        {
            lightingPass.width = swapchainFramebufferWidth;
            lightingPass.height = swapchainFramebufferHeight;
            lightingPass.dependencies[0] = shadowMappingPass.depthTexture;
            lightingPass.dependencyCount = 1;
            RenderPassLighting(window, lightingPass, models);
        }

        {
            taaPass.width = swapchainFramebufferWidth;
            taaPass.height = swapchainFramebufferHeight;
            taaPass.dependencies[0] = lightingPass.colorTexture;
            taaPass.dependencies[1] = lightingPass.depthTexture;
            taaPass.dependencies[2] = g_taaBuffer.historyTexture;
            taaPass.dependencyCount = 3;
            RenderPassTAA(taaPass);
        }

        {
            debugPass.width = swapchainFramebufferWidth;
            debugPass.height = swapchainFramebufferHeight;
            debugPass.dependencies[0] = lightingPass.colorTexture;
            debugPass.dependencies[1] = lightingPass.depthTexture;
            debugPass.dependencies[2] = g_taaBuffer.drawTexture;
            debugPass.dependencies[3] = g_taaBuffer.historyTexture;
            debugPass.dependencyCount = 4;
            RenderPassDebug(debugPass);
        }

        {
            ExectureBackBufferBlitRenderPass(debugPass, GL_COLOR_ATTACHMENT0);
        }

        if (g_drawUi)
        {
            DrawUI(window);
        }

        glfwSwapBuffers(window);
    }

    glDeleteFramebuffers(1, &lightingPass.fbo1);
    glDeleteFramebuffers(1, &shadowMappingPass.fbo1);
    glDeleteFramebuffers(1, &taaPass.fbo1);
}

int main()
{
    GLFWwindow *window = InitializeGLFW(g_defaultWidth, g_defaultHeight);
    InitializeImGui(window);

    SetupGLFWCallbacks(window);
    ConfigureGL();
    InitializeGlobals();

    MainLoop(window);

    DeinitializeImGui();
    DeinitializeGLFW(window);

    return 0;
}
