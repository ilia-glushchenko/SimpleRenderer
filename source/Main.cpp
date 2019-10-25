/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#include "RenderDefinitions.hpp"
#include "RenderConfiguration.hpp"
#include "RenderPass.hpp"
#include "RenderModel.hpp"
#include "Camera.hpp"
#include "Math.hpp"
#include "Loader.hpp"
#include "TestModels.hpp"

#include <ctime>
#include <chrono>
#include <type_traits>

Camera g_camera = CreateCamera();
DirectionalLightSource g_directLight = {
    sr::math::CreateOrthographicProjectionMatrix(-2048.f, 2048.f, -2048.f, 2048.f, -2000.f, 1500.f),
    sr::math::CreateRotationMatrixX(-1.57f)};
constexpr uint32_t g_defaultWidth = 800;
constexpr uint32_t g_defaultHeight = 800;
TAABuffer g_taaBuffer = {};

bool g_captureMouse = false;
bool g_drawUi = false;
bool g_isHotRealoadRequired = false;
uint32_t g_shadowMapsMode = 1;
uint32_t g_bumpMappingEnabled = 1;
float g_bumpMapScaleFactor = 0.00001f;
uint32_t g_bumpMapAvailable = 0;
uint32_t g_metallicMapAvailable = 0;
uint32_t g_roughnessMapAvailable = 0;
constexpr int32_t g_pointLightCount = 5;
sr::math::Vec3 g_pointLights[g_pointLightCount] = {
    {-1200, 200, -45},
    {-700, 200, -45},
    {0, 200, -45},
    {700, 200, -45},
    {1100, 200, -45}};
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
    "RoughnessMap"};

sr::math::Matrix4x4 CreateCameraMatrix(sr::math::Vec3 pos, float xWorldAngle, float yWorldAngle)
{
    return sr::math::Mul(
        sr::math::CreateTranslationMatrix(pos.x, pos.y, pos.z),
        sr::math::Mul(
            sr::math::CreateRotationMatrixY(yWorldAngle),
            sr::math::CreateRotationMatrixX(xWorldAngle)));
}

sr::math::Matrix4x4 CreateViewMatrix(sr::math::Vec3 pos, float xWorldAngle, float yWorldAngle)
{
    return sr::math::Mul(
        sr::math::Mul(
            sr::math::CreateRotationMatrixX(-xWorldAngle),
            sr::math::CreateRotationMatrixY(-yWorldAngle)),
        sr::math::CreateTranslationMatrix(-pos.x, -pos.y, -pos.z));
}

void GLFWKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto &io = ImGui::GetIO();

    sr::math::Vec4 forward = {};
    sr::math::Vec4 right = {};
    sr::math::Matrix4x4 const camera = CreateCameraMatrix(
        g_camera.pos, g_camera.xWorldAngle, g_camera.yWorldAngle);

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_W:
            forward = sr::math::Mul(camera, sr::math::Vec4{0, 0, -1, 0});
            forward *= 5.0f;
            break;
        case GLFW_KEY_S:
            forward = sr::math::Mul(camera, sr::math::Vec4{0, 0, -1, 0});
            forward *= -5.0f;
            break;
        case GLFW_KEY_A:
            right = sr::math::Mul(camera, sr::math::Vec4{-1, 0, 0, 0});
            right *= 5.05f;
            break;
        case GLFW_KEY_D:
            right = sr::math::Mul(camera, sr::math::Vec4{-1, 0, 0, 0});
            right *= -5.05f;
            break;
        case GLFW_KEY_Q:
            g_camera.pos.y -= 5.05f;
            break;
        case GLFW_KEY_E:
            g_camera.pos.y += 5.05f;
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

    g_camera.pos += forward.xyz;
    g_camera.pos += right.xyz;
}

void GLFWWindowSizeCallback(GLFWwindow *window, int width, int height)
{
    assert(window != nullptr);

    if (width > 0 && height > 0)
    {
        g_camera.fov = 1.0472f;
        g_camera.aspect = static_cast<float>(width) / height;

        g_isHotRealoadRequired = true;
    }
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

        g_camera.xWorldAngle = xWorldAngle;
        g_camera.yWorldAngle = yWorldAngle;
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
    ImGui::InputFloat3("Camera pos:", g_camera.pos.data, 1);
    ImGui::InputFloat2("Camera XY rads:", &g_camera.xWorldAngle, 1);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::vector<RenderModel> LoadModels(ShaderProgram const &program)
{
    std::vector<RenderModel> models;
    std::vector<sr::load::Geometry> geometries;
    std::vector<sr::load::MaterialSource> materials;

    sr::load::LoadOBJ("data\\models\\Sponza", "sponza.obj", geometries, materials);
    for (uint32_t i = 0; i < geometries.size(); ++i)
    {
        models.push_back(CreateRenderModel(
            sr::load::CreateBufferDescriptors(geometries[i]),
            sr::load::CreateIndexBufferDescriptor(geometries[i]),
            materials[geometries[i].material]));
        LinkRenderModelToShaderProgram(
            program.handle, models[i], g_shaderAttributesPositionNormalUV);
    }

    for (uint32_t i = 0; i < g_pointLightCount; ++i)
    {
        sr::load::LoadOBJ("data\\models\\box", "box.obj", geometries, materials);
        models.push_back(
            CreateRenderModel(
                sr::load::CreateBufferDescriptors(geometries.back()),
                sr::load::CreateIndexBufferDescriptor(geometries.back()),
                {}));
        LinkRenderModelToShaderProgram(
            program.handle, models.back(), g_shaderAttributesPositionNormalUV);
        models.back().color = sr::math::Vec3{1.f, 209 / 255.0f, 163 / 255.0f};
        models.back().debugRenderModel = 1;
        models.back().model = sr::math::Mul(
            sr::math::CreateTranslationMatrix(g_pointLights[i]),
            sr::math::CreateScaleMatrix(10));
    }

    {
        tinyobj::material_t material;
        material.diffuse_texname = "shfsaida_2K_Albedo.jpg";
        material.bump_texname = "shfsaida_2K_Bump.jpg";
        material.normal_texname = "shfsaida_2K_Normal.jpg";
        material.roughness_texname = "shfsaida_2K_Roughness.jpg";
        sr::load::LoadOBJ("data\\models\\quad", "quad.obj", geometries, materials);
        models.push_back(CreateRenderModel(
            sr::load::CreateBufferDescriptors(geometries.back()),
            sr::load::CreateIndexBufferDescriptor(geometries.back()),
            sr::load::CreateMaterialSource(
                "data\\materials\\2k\\Rock_Cliffs_shfsaida_2K_surface_ms", material)));
        models.back().brdf = 0;
        LinkRenderModelToShaderProgram(
            program.handle, models.back(), g_shaderAttributesPositionNormalUV);
        models.back().model = sr::math::Mul(
            sr::math::CreateTranslationMatrix(0, 50.f, 0),
            sr::math::Mul(
                sr::math::CreateScaleMatrix(100.f),
                sr::math::CreateRotationMatrixY(6.28f * 0.65f)));
    }

    for (auto &material : materials)
    {
        FreeMaterialSource(material);
    }

    return models;
}

PipelineShaderPrograms CreatePipelineShaderPrograms()
{
    PipelineShaderPrograms desc;

    desc.depthPrePass = CreateShaderProgram("shaders/depth_pre_pass.vert", "shaders/depth_pre_pass.frag");
    desc.shadowMapping = CreateShaderProgram("shaders/shadow_mapping.vert", "shaders/shadow_mapping.frag");
    desc.lighting = CreateShaderProgram("shaders/lighting.vert", "shaders/lighting.frag");
    desc.velocity = CreateShaderProgram("shaders/velocity.vert", "shaders/velocity.frag");
    desc.toneMapping = CreateShaderProgram("shaders/tone_mapping.vert", "shaders/tone_mapping.frag");
    LinkRenderModelToShaderProgram(desc.lighting.handle, g_quadWallRenderModel, g_shaderAttributesPositionNormalUV);
    desc.taa = CreateShaderProgram("shaders/taa.vert", "shaders/taa.frag");
    LinkRenderModelToShaderProgram(desc.taa.handle, g_quadWallRenderModel, g_shaderAttributesPositionNormalUV);
    desc.debug = CreateShaderProgram("shaders/debug.vert", "shaders/debug.frag");
    LinkRenderModelToShaderProgram(desc.debug.handle, g_quadWallRenderModel, g_shaderAttributesPositionNormalUV);

    return desc;
}

void CreatePipelineUniformBindngs(PipelineShaderPrograms &desc, std::vector<RenderModel> const &models)
{
    {
        CreateShaderProgramUniformBindings(
            desc.depthPrePass,
            UniformsDescriptor{
                UniformsDescriptor::UI32{},
                UniformsDescriptor::F1{},
                UniformsDescriptor::F2{},
                UniformsDescriptor::F3{},
                UniformsDescriptor::F4{},
                UniformsDescriptor::MAT4{
                    {"uProjMat", "uViewMat"},
                    {g_camera.proj.data, g_camera.view.data},
                    {1, 1}},
                UniformsDescriptor::ArrayUI32{},
                UniformsDescriptor::ArrayF1{},
                UniformsDescriptor::ArrayF2{},
                UniformsDescriptor::ArrayF3{},
                UniformsDescriptor::ArrayF4{},
                UniformsDescriptor::ArrayMAT4{
                    {"uModelMat"},
                    {reinterpret_cast<float const *>(models.data())},
                    {offsetof(RenderModel, RenderModel::model)},
                    {sizeof(RenderModel)}},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.shadowMapping,
            UniformsDescriptor{
                UniformsDescriptor::UI32{},
                UniformsDescriptor::F1{},
                UniformsDescriptor::F2{},
                UniformsDescriptor::F3{},
                UniformsDescriptor::F4{},
                UniformsDescriptor::MAT4{
                    {"uProjMat", "uViewMat"},
                    {g_directLight.projection.data, g_directLight.view.data},
                    {1, 1}},
                UniformsDescriptor::ArrayUI32{},
                UniformsDescriptor::ArrayF1{},
                UniformsDescriptor::ArrayF2{},
                UniformsDescriptor::ArrayF3{},
                UniformsDescriptor::ArrayF4{},
                UniformsDescriptor::ArrayMAT4{
                    {"uModelMat"},
                    {reinterpret_cast<float const *>(models.data())},
                    {offsetof(RenderModel, RenderModel::model)},
                    {sizeof(RenderModel)}},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.lighting,
            UniformsDescriptor{
                //uint32_t
                UniformsDescriptor::UI32{
                    {"uRenderModeUint",
                     "uShadowMappingEnabledUint",
                     "uBumpMappingEnabledUint"},
                    {reinterpret_cast<uint32_t *>(&g_renderMode),
                     &g_shadowMapsMode,
                     &g_bumpMappingEnabled},
                    {1, 1, 1}},
                //floats
                UniformsDescriptor::F1{
                    {"uBumpMapScaleFactorFloat"},
                    {&g_bumpMapScaleFactor},
                    {1}},
                UniformsDescriptor::F2{},
                //float3
                UniformsDescriptor::F3{
                    {"uCameraPos", "uPointLightPosVec3Array"},
                    {g_camera.pos.data,
                     g_pointLights[0].data},
                    {1, g_pointLightCount}},
                UniformsDescriptor::F4{},
                //mat4
                UniformsDescriptor::MAT4{
                    {"uProjMat",
                     "uViewMat",
                     "uDirLightProjMat",
                     "uDirLightViewMat"},
                    {g_camera.proj.data,
                     g_camera.view.data,
                     g_directLight.projection.data,
                     g_directLight.view.data},
                    {1, 1, 1, 1}},
                //uint32_t array
                UniformsDescriptor::ArrayUI32{
                    {"uBumpMapAvailableUint",
                     "uMetallicMapAvailableUint",
                     "uRoughnessMapAvailableUint",
                     "uDebugRenderModeAvailableUint",
                     "uBrdfUint"},
                    {reinterpret_cast<uint32_t const *>(models.data()),
                     reinterpret_cast<uint32_t const *>(models.data()),
                     reinterpret_cast<uint32_t const *>(models.data()),
                     reinterpret_cast<uint32_t const *>(models.data()),
                     reinterpret_cast<uint32_t const *>(models.data())},
                    {offsetof(RenderModel, RenderModel::bumpTexture),
                     offsetof(RenderModel, RenderModel::metallicTexture),
                     offsetof(RenderModel, RenderModel::roughnessTexture),
                     offsetof(RenderModel, RenderModel::debugRenderModel),
                     offsetof(RenderModel, RenderModel::brdf)},
                    {sizeof(RenderModel),
                     sizeof(RenderModel),
                     sizeof(RenderModel),
                     sizeof(RenderModel),
                     sizeof(RenderModel)}},
                //float1
                UniformsDescriptor::ArrayF1{},
                UniformsDescriptor::ArrayF2{},
                //float3
                UniformsDescriptor::ArrayF3{
                    {"uColor"},
                    {reinterpret_cast<float const *>(models.data())},
                    {offsetof(RenderModel, RenderModel::color)},
                    {sizeof(RenderModel)}},
                UniformsDescriptor::ArrayF4{},
                //mat4
                UniformsDescriptor::ArrayMAT4{
                    {"uModelMat"},
                    {reinterpret_cast<float const *>(models.data())},
                    {offsetof(RenderModel, RenderModel::model)},
                    {sizeof(RenderModel)}},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.velocity,
            UniformsDescriptor{
                UniformsDescriptor::UI32{},
                UniformsDescriptor::F1{},
                UniformsDescriptor::F2{},
                UniformsDescriptor::F3{},
                UniformsDescriptor::F4{},
                UniformsDescriptor::MAT4{
                    {"uPrevViewMat4",
                     "uPrevProjMat4",
                     "uViewMat4",
                     "uProjMat4"},
                    {g_taaBuffer.prevView.data,
                     g_taaBuffer.prevProj.data,
                     g_camera.view.data,
                     g_camera.proj.data},
                    {1, 1, 1, 1},
                },
                UniformsDescriptor::ArrayUI32{},
                UniformsDescriptor::ArrayF1{},
                UniformsDescriptor::ArrayF2{},
                UniformsDescriptor::ArrayF3{},
                UniformsDescriptor::ArrayF4{},
                UniformsDescriptor::ArrayMAT4{
                    {"uPrevModelMat4", "uModelMat4"},
                    {reinterpret_cast<float const *>(g_taaBuffer.prevModels.data()),
                     reinterpret_cast<float const *>(models.data())},
                    {0, offsetof(RenderModel, RenderModel::model)},
                    {sizeof(sr::math::Matrix4x4), sizeof(RenderModel)},
                },
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.toneMapping,
            UniformsDescriptor{
                UniformsDescriptor::UI32{},
                UniformsDescriptor::F1{},
                UniformsDescriptor::F2{},
                UniformsDescriptor::F3{},
                UniformsDescriptor::F4{},
                UniformsDescriptor::MAT4{},
                UniformsDescriptor::ArrayUI32{},
                UniformsDescriptor::ArrayF1{},
                UniformsDescriptor::ArrayF2{},
                UniformsDescriptor::ArrayF3{},
                UniformsDescriptor::ArrayF4{},
                UniformsDescriptor::ArrayMAT4{},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.taa,
            UniformsDescriptor{
                UniformsDescriptor::UI32{
                    {"uFrameCountUint"}, {&g_taaBuffer.count}, {1}},
                UniformsDescriptor::F1{},
                UniformsDescriptor::F2{
                    {"uJitterVec2"}, {g_taaBuffer.jitter.data}, {1}},
                UniformsDescriptor::F3{},
                UniformsDescriptor::F4{},
                UniformsDescriptor::MAT4{
                    {"uViewMat",
                     "uProjMat",
                     "uPrevViewMat",
                     "uPrevProjMat"},
                    {g_camera.view.data,
                     g_camera.proj.data,
                     g_taaBuffer.prevView.data,
                     g_taaBuffer.prevProj.data},
                    {1, 1, 1, 1}},
                UniformsDescriptor::ArrayUI32{},
                UniformsDescriptor::ArrayF1{},
                UniformsDescriptor::ArrayF2{},
                UniformsDescriptor::ArrayF3{},
                UniformsDescriptor::ArrayF4{},
                UniformsDescriptor::ArrayMAT4{},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.debug,
            UniformsDescriptor{
                UniformsDescriptor::UI32{},
                UniformsDescriptor::F1{},
                UniformsDescriptor::F2{},
                UniformsDescriptor::F3{},
                UniformsDescriptor::F4{},
                UniformsDescriptor::MAT4{},
                UniformsDescriptor::ArrayUI32{},
                UniformsDescriptor::ArrayF1{},
                UniformsDescriptor::ArrayF2{},
                UniformsDescriptor::ArrayF3{},
                UniformsDescriptor::ArrayF4{},
                UniformsDescriptor::ArrayMAT4{},
            });
    }
}

void UpdateModels(std::vector<RenderModel> &models)
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t const time = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    float const s = std::sin(static_cast<float>(time));
    models.back().model = sr::math::Mul(
        sr::math::CreateRotationMatrixY(0.01f),
        models.back().model);
}

void RenderPassDepthPrePass(Pipeline &pipeline, std::vector<RenderModel> const &models)
{
    g_camera.view = CreateViewMatrix(g_camera.pos, g_camera.xWorldAngle, g_camera.yWorldAngle);
    g_camera.proj = sr::math::CreatePerspectiveProjectionMatrix(
        g_camera.near, g_camera.far, g_camera.fov, g_camera.aspect);
    // g_camera.proj = sr::math::CreateOrthographicProjectionMatrix(
    //     -2048.f, 2048.f, -2048.f, 2048.f, -2000.f, 1500.f);
    const uint32_t taaSampleIndex = g_taaBuffer.count % 16;

    // https://github.com/playdeadgames/temporal
    {
        float const oneExtentY = std::tan(0.5f * g_camera.fov);
        float const oneExtentX = oneExtentY * g_camera.aspect;
        float const texelSizeX = oneExtentX / (0.5f * pipeline.debug.width);
        float const texelSizeY = oneExtentY / (0.5f * pipeline.debug.height);
        float const oneJitterX = texelSizeX * g_taaHalton23Sequence16[taaSampleIndex].x;
        float const oneJitterY = texelSizeY * g_taaHalton23Sequence16[taaSampleIndex].y;

        float const cf = g_camera.far;
        float const cn = g_camera.near;
        float const xm = oneJitterX - oneExtentX;
        float const xp = oneJitterX + oneExtentX;
        float const ym = oneJitterY - oneExtentY;
        float const yp = oneJitterY + oneExtentY;

        g_taaBuffer.jitter = {oneJitterX, oneJitterY};
        //Note: Below is Inside approach, might be better that mine
        //g_camera.proj = sr::math::CreatePerspectiveProjectionMatrix(
        //    xm * cn, xp * cn, ym * cn, yp * cn, cn, cf);
        g_camera.proj = sr::math::Mul(
            sr::math::CreateTranslationMatrix(oneJitterX, oneJitterY, 0),
            g_camera.proj);
    }

    int32_t const width = pipeline.depthPrePass.width;
    int32_t const height = pipeline.depthPrePass.height;
    sr::math::Vec2 const sample = g_taaHalton23Sequence16[taaSampleIndex] - 0.5f;

    ExecuteRenderPass(pipeline.depthPrePass, models.data(), models.size());
}

void RenderPassShadowMap(Pipeline &pipeline, std::vector<RenderModel> const &models)
{
    ExecuteRenderPass(pipeline.shadowMapping, models.data(), models.size());
}

void RenderPassLighting(Pipeline &pipeline, std::vector<RenderModel> const &models)
{
    ExecuteRenderPass(pipeline.lighting, models.data(), models.size());
}

void RenderPassVelocity(Pipeline &pipeline, std::vector<RenderModel> const &models)
{
    ExecuteRenderPass(pipeline.velocity, models.data(), models.size());

    g_taaBuffer.prevModels.resize(models.size());
    for (uint64_t i = 0; i < models.size(); ++i)
    {
        g_taaBuffer.prevModels[i] = models[i].model;
    }
}

void RenderPassToneMapping(Pipeline &pipeline)
{
    ExecuteRenderPass(pipeline.toneMapping, &g_quadWallRenderModel, 1);
}

void RenderPassTAA(Pipeline &pipeline)
{
    ExecuteRenderPass(pipeline.taa, &g_quadWallRenderModel, 1);

    pipeline.taa.subPasses[0].active = !pipeline.taa.subPasses[0].active;
    pipeline.taa.subPasses[1].active = !pipeline.taa.subPasses[0].active;

    g_taaBuffer.prevProj = g_camera.proj;
    g_taaBuffer.prevView = g_camera.view;
    g_taaBuffer.count++;
}

void RenderPassDebug(Pipeline &pipeline)
{
    ExecuteRenderPass(pipeline.debug, &g_quadWallRenderModel, 1);

    std::swap(pipeline.debug.subPasses[0].desc.dependencies[4].handle,
              pipeline.debug.subPasses[0].desc.dependencies[5].handle);
}

void MainLoop(GLFWwindow *window)
{
    static int swapchainFramebufferWidth = 0, swapchainFramebufferHeight = 0;
    glfwGetFramebufferSize(window, &swapchainFramebufferWidth, &swapchainFramebufferHeight);

    std::vector<sr::load::Geometry> geometries;
    std::vector<sr::load::MaterialSource> materials;
    auto programs = CreatePipelineShaderPrograms();
    auto models = LoadModels(programs.lighting);
    g_taaBuffer.prevModels.resize(models.size());
    CreatePipelineUniformBindngs(programs, models);
    auto pipeline = CreateRenderPipeline(programs, swapchainFramebufferWidth, swapchainFramebufferHeight);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glfwGetFramebufferSize(window, &swapchainFramebufferWidth, &swapchainFramebufferHeight);

        if (g_isHotRealoadRequired)
        {
            DeleteRenderPipeline(pipeline);

            programs = CreatePipelineShaderPrograms();
            CreatePipelineUniformBindngs(programs, models);
            pipeline = CreateRenderPipeline(programs, swapchainFramebufferWidth, swapchainFramebufferHeight);

            std::time_t const timestamp = std::time(nullptr);
            std::cout << "Backbuffer size: " << swapchainFramebufferWidth << "x" << swapchainFramebufferHeight
                      << "\nHot reload: " << std::asctime(std::localtime(&timestamp)) << std::endl;

            g_isHotRealoadRequired = false;
        }

        UpdateModels(models);
        RenderPassDepthPrePass(pipeline, models);
        RenderPassShadowMap(pipeline, models);
        RenderPassLighting(pipeline, models);
        RenderPassVelocity(pipeline, models);
        RenderPassToneMapping(pipeline);
        RenderPassTAA(pipeline);
        RenderPassDebug(pipeline);

        ExecuteBackBufferBlitRenderPass(
            pipeline.debug.subPasses[0].fbo,
            GL_COLOR_ATTACHMENT0,
            swapchainFramebufferWidth,
            swapchainFramebufferHeight);

        if (g_drawUi)
        {
            DrawUI(window);
        }

        glfwSwapBuffers(window);
    }
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
