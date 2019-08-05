/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#include "Math.hpp"
#include "Loader.hpp"
#include "Renderer.hpp"
#include "TestModels.hpp"

Camera g_camera = CreateCamera();
DirectionalLightSource g_directLight = {
    sr::math::CreateOrthographicProjectionMatrix(-2048.f, 2048.f, -2048.f, 2048.f, 1.f, 2000.f),
    sr::math::CreateChangeOfBasisMatrix({0, -1, 0}, {0, 0, -1}, {1, 0, 0}, {0, 1000.f, 0.f}),
};

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
float g_lightPosition[3] = {0, 20, -45};
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

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
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

void WindowSizeCallback(GLFWwindow *window, int width, int height)
{
    g_camera.proj = sr::math::CreatePerspectiveProjectionMatrix(0.1f, 10000.f, 1.0472f, static_cast<float>(width) / height);
    g_isPipelineReloadRequired = true;
}

void CursorPosCallback(GLFWwindow *window, double x, double y)
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
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetWindowSizeCallback(window, WindowSizeCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
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
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::vector<RenderModel> LoadModels(GLuint program)
{
    std::vector<AttributeDescriptor> const attributes = {
        {"aPosition", program, 3, sizeof(sr::math::Vec3)},
        {"aNormal", program, 3, sizeof(sr::math::Vec3)},
        {"aUV", program, 2, sizeof(sr::math::Vec2)},
    };

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
        models.push_back(CreateRenderModel(attributes,
                                           sr::load::CreateBufferDescriptors(geometries[i]),
                                           sr::load::CreateIndexBufferDescriptor(geometries[i]),
                                           materials[geometries[i].material]));
    }

    for (auto &material : materials)
    {
        sr::load::FreeMaterialSource(material);
    }

    return models;
}

RenderPass CreateLightingRenderPass(GLFWwindow *window, ShaderProgram program)
{
    static int displayWidth = 0, displayHeight = 0;
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);

    RenderPass lightingRenderPass;
    glGenFramebuffers(1, &lightingRenderPass.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, lightingRenderPass.fbo);
    {
        lightingRenderPass.program = program;
        lightingRenderPass.colorTexture = CreateTexture(
            sr::load::TextureSource{"", nullptr, displayWidth, displayHeight, 4, GL_RGBA});
        lightingRenderPass.depthTexture = CreateDepthTexture(displayWidth, displayHeight);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lightingRenderPass.colorTexture, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, lightingRenderPass.depthTexture, 0);
        glDrawBuffers(1, &GL_COLOR_ATTACHMENT0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    { //Check for FBO completeness
        std::cerr << "Error! FrameBuffer is not complete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return lightingRenderPass;
}

RenderPass CreateLightingRenderPass(GLFWwindow *window)
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
                {"uPointLightPos"},
                {g_lightPosition}},
            //mat4
            UniformsDescriptor::MAT4{
                {"uProjMat", "uViewMat", "uDirLightProjMat", "uDirLightViewMat"},
                {g_camera.proj.data, g_camera.view.data, g_directLight.projection.data, g_directLight.view.data}}});

    return CreateLightingRenderPass(window, program);
}

RenderPass CreateShadowMappingRenderPass(GLFWwindow *window, ShaderProgram program)
{
    RenderPass shadowMappingPass;
    shadowMappingPass.program = program;
    glGenFramebuffers(1, &shadowMappingPass.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMappingPass.fbo);
    {
        shadowMappingPass.depthTexture = CreateDepthTexture(4096, 4096);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMappingPass.depthTexture, 0);
        glDrawBuffers(1, &GL_NONE);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    { //Check for FBO completeness
        std::cerr << "Error! FrameBuffer is not complete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return shadowMappingPass;
}

RenderPass CreateShadowMappingRenderPass(GLFWwindow *window)
{
    auto program = CreateShaderProgram(
        "shaders/shadow_mapping.vert", "shaders/shadow_mapping.frag",
        UniformsDescriptor{
            {}, {}, {}, UniformsDescriptor::MAT4{{"uProjMat", "uViewMat"}, {g_directLight.projection.data, g_directLight.view.data}}});

    return CreateShadowMappingRenderPass(window, program);
}

RenderPass CreateTaaRenderPass(GLFWwindow *window, ShaderProgram program)
{
    static int displayWidth = 0, displayHeight = 0;
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);

    RenderPass taaPass;
    taaPass.colorTexture = CreateTexture(
        sr::load::TextureSource{"", nullptr, displayWidth, displayHeight, 4, GL_RGBA});
    taaPass.program = program;
    glGenFramebuffers(1, &taaPass.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, taaPass.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return taaPass;
}

RenderPass CreateTaaRenderPass(GLFWwindow *window)
{
    auto program = CreateShaderProgram(
        "shaders/taa.vert", "shaders/taa.frag",
        UniformsDescriptor{
            {},
            {},
            {},
            {},
        });

    return CreateTaaRenderPass(window, program);
}

void RenderPassLighting(GLFWwindow *window, GLuint shadowMapTexture, RenderPass const &pass, std::vector<RenderModel> const &models)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);

    glUseProgram(pass.program.handle);
    static int displayWidth = 0, displayHeight = 0;
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight);
    glScissor(0, 0, displayWidth, displayHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_camera.jitter = sr::math::CreateUniformRandomVec3(-1, +1);
    g_camera.view = CreateViewMatrix(g_camera.pos + g_camera.jitter, g_camera.yWorldAndle);

    auto const colorVectorMatrix = glGetUniformLocation(pass.program.handle, "uColor");
    auto const cameraPosVector = glGetUniformLocation(pass.program.handle, "uCameraPos");
    auto const modelMatrixLocation = glGetUniformLocation(pass.program.handle, "uModelMat");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

    for (uint32_t i = 0; i < models.size(); ++i)
    {
        glBindVertexArray(models[i].vertexArrayObject);

        { // Uniforms update
            g_bumpMapAvailable = models[i].bumpTexture;
            g_metallicMapAvailable = models[i].metallicTexture;
            g_roughnessMapAvailable = models[i].roughnessTexture;

            for (auto &uniform : pass.program.unifromsui32)
            {
                glUniform1ui(uniform.location, *uniform.data);
            }
            for (auto &uniform : pass.program.uniformsf)
            {
                glUniform1f(uniform.location, *uniform.data);
            }
            for (auto &uniform : pass.program.uniforms3f)
            {
                glUniform3fv(uniform.location, 1, uniform.data);
            }
            for (auto &uniform : pass.program.uniforms16f)
            {
                glUniformMatrix4fv(uniform.location, 1, GL_TRUE, uniform.data);
            }

            glUniform3fv(cameraPosVector, 1, g_camera.pos.data);
            glUniform3fv(colorVectorMatrix, 1, models[i].color.data);
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_TRUE, models[i].model.data);
        }

        if (models[i].albedoTexture != 0)
        {
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, models[i].albedoTexture);
        }
        if (models[i].normalTexture != 0)
        {
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D, models[i].normalTexture);
        }
        if (models[i].bumpTexture != 0)
        {
            glActiveTexture(GL_TEXTURE0 + 3);
            glBindTexture(GL_TEXTURE_2D, models[i].bumpTexture);
        }
        if (models[i].metallicTexture != 0)
        {
            glActiveTexture(GL_TEXTURE0 + 4);
            glBindTexture(GL_TEXTURE_2D, models[i].metallicTexture);
        }
        if (models[i].roughnessTexture != 0)
        {
            glActiveTexture(GL_TEXTURE0 + 5);
            glBindTexture(GL_TEXTURE_2D, models[i].roughnessTexture);
        }

        glDrawElements(GL_TRIANGLES, models[i].indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPassShadowMap(GLFWwindow *window, RenderPass const &pass, std::vector<RenderModel> const &models)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);

    glUseProgram(pass.program.handle);
    glViewport(0, 0, 4096, 4096);
    glScissor(0, 0, 4096, 4096);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_camera.view = CreateViewMatrix(g_camera.pos, g_camera.yWorldAndle);

    auto const modelMatrixLocation = glGetUniformLocation(pass.program.handle, "uModelMat");

    for (auto &model : models)
    {
        { // Uniforms update
            for (auto &uniform : pass.program.uniforms16f)
            {
                glUniformMatrix4fv(uniform.location, 1, GL_TRUE, uniform.data);
            }
            for (auto &uniform : pass.program.uniforms3f)
            {
                glUniform3fv(uniform.location, 1, uniform.data);
            }

            glUniformMatrix4fv(modelMatrixLocation, 1, GL_TRUE, model.model.data);
        }

        glBindVertexArray(model.vertexArrayObject);
        glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPassTAA(GLFWwindow *window, GLuint colorTexture, GLuint depthTexture, RenderPass const &pass)
{
    //glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);

    static std::vector<AttributeDescriptor> const attributes = {
        {"aPosition", pass.program.handle, 3, sizeof(sr::math::Vec3)},
        {"aNormal", pass.program.handle, 3, sizeof(sr::math::Vec3)},
        {"aUV", pass.program.handle, 2, sizeof(sr::math::Vec2)},
    };
    static RenderModel model = CreateRenderModel(attributes,
                                                 sr::load::CreateBufferDescriptors(g_quadWall),
                                                 sr::load::CreateIndexBufferDescriptor(g_quadWall),
                                                 {});

    static int displayWidth = 0, displayHeight = 0;
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
    static TAABuffer taaBuffer = CreateTAABuffer(displayWidth, displayHeight, 2);
    static uint32_t current = 0;

    glUseProgram(pass.program.handle);
    glViewport(0, 0, displayWidth, displayHeight);
    glScissor(0, 0, displayWidth, displayHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glBindVertexArray(model.vertexArrayObject);
    glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, nullptr);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, displayWidth, displayHeight);

    current = current + 1 == taaBuffer.count ? current = 0 : current + 1;

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ConfigureGL()
{
    glClearColor(1, 1, 1, 1);
    glClearDepth(1.0f);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
}

void MainLoop(GLFWwindow *window)
{
    auto lightingPass = CreateLightingRenderPass(window);
    auto shadowMappingPass = CreateShadowMappingRenderPass(window);
    auto taaPass = CreateTaaRenderPass(window);

    auto models = LoadModels(lightingPass.program.handle);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (g_isHotRealoadRequired)
        {
            DeleteShaderProgram(lightingPass.program);
            DeleteShaderProgram(shadowMappingPass.program);
            DeleteShaderProgram(taaPass.program);

            lightingPass = CreateLightingRenderPass(window);
            shadowMappingPass = CreateShadowMappingRenderPass(window);
            taaPass = CreateTaaRenderPass(window);

            std::cout << "Hot reloaded" << std::endl;

            g_isHotRealoadRequired = false;
            g_isPipelineReloadRequired = false;
        }
        else if (g_isPipelineReloadRequired)
        {
            lightingPass = CreateLightingRenderPass(window, lightingPass.program);
            shadowMappingPass = CreateShadowMappingRenderPass(window, shadowMappingPass.program);
            taaPass = CreateTaaRenderPass(window, taaPass.program);

            g_isPipelineReloadRequired = false;
        }

        RenderPassShadowMap(window, shadowMappingPass, models);
        RenderPassLighting(window, shadowMappingPass.depthTexture, lightingPass, models);
        RenderPassTAA(window, lightingPass.colorTexture, lightingPass.depthTexture, taaPass);

        if (g_drawUi)
        {
            DrawUI(window);
        }
        glfwSwapBuffers(window);
    }

    glDeleteFramebuffers(1, &lightingPass.fbo);
    glDeleteFramebuffers(1, &shadowMappingPass.fbo);
    glDeleteFramebuffers(1, &taaPass.fbo);
}

int main()
{
    GLFWwindow *window = InitializeGLFW();
    InitializeImGui(window);

    SetupGLFWCallbacks(window);
    ConfigureGL();
    MainLoop(window);

    DeinitializeImGui();
    DeinitializeGLFW(window);

    return 0;
}
