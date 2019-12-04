/*
 * Copyright (C) 2019 by Ilya Glushchenko
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */
#include "Camera.hpp"
#include "Input.hpp"
#include "Loader.hpp"
#include "Math.hpp"
#include "RenderConfiguration.hpp"
#include "RenderDefinitions.hpp"
#include "RenderModel.hpp"
#include "RenderPass.hpp"
#include "RenderPipeline.hpp"
#include "TestModels.hpp"

#include <ctime>

constexpr uint32_t g_defaultWidth = 800;
constexpr uint32_t g_defaultHeight = 800;

Camera g_camera = CreateCamera();
float g_cameraSpeed = 5;
Camera g_prevCamera = CreateCamera();

bool g_captureMouse = false;
bool g_drawUi = true;
bool g_isHotRealoadRequired = false;
bool g_drawAABBs = false;

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
    "Normal Maps",
    "Bump Maps",
    "Depth Buffer",
    "Shadow Maps",
    "Metallic Maps",
    "Roughness Maps"};

uint32_t g_directLightEnabled = 1;
uint32_t g_shadowMappingEnabled = 1;
uint32_t g_pointLightEnabled = 0;
uint32_t g_bumpMappingEnabled = 0;
uint32_t g_toneMappingEnabled = 0;
uint32_t g_taaEnabled = 0;
uint32_t g_taaJitterEnabled = 0;

uint32_t g_bumpMapAvailable = 0;
uint32_t g_metallicMapAvailable = 0;
uint32_t g_roughnessMapAvailable = 0;

float g_bumpMapScaleFactor = 0.00001f;
float g_depthBiasScale = 0.1f;
float g_depthUnitScale = 1.0f;
TAABuffer g_taaBuffer = {};

float g_ambientLightRadiantFlux = 0.5f;
DirectionalLightSource g_directLight;
float g_pointLightRadiantFlux = 2.5f;
constexpr int32_t g_pointLightCount = 5;
sr::math::Vec3 g_pointLights[g_pointLightCount] = {
    {-1200, 200, -45},
    {-700, 200, -45},
    {0, 200, -45},
    {700, 200, -45},
    {1100, 200, -45}};

sr::math::Matrix4x4 CreateCameraMatrix(sr::math::Vec3 pos, float xWorldAngle, float yWorldAngle)
{
    return sr::math::CreateTranslationMatrix(pos.x, pos.y, pos.z) *
           sr::math::CreateRotationMatrixY(yWorldAngle) *
           sr::math::CreateRotationMatrixX(xWorldAngle);
}

sr::math::Matrix4x4 CreateViewMatrix(sr::math::Vec3 pos, float xWorldAngle, float yWorldAngle)
{
    return sr::math::CreateRotationMatrixX(-xWorldAngle) *
           sr::math::CreateRotationMatrixY(-yWorldAngle) *
           sr::math::CreateTranslationMatrix(-pos.x, -pos.y, -pos.z);
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
    glfwSetKeyCallback(window, sr::input::GLFWKeyCallback);
    glfwSetWindowSizeCallback(window, GLFWWindowSizeCallback);
    glfwSetCursorPosCallback(window, GLFWCursorPosCallback);
}

void DrawUI(GLFWwindow *window)
{
#ifdef NDEBUG
    glPushGroupMarkerEXT(2, "UI");
#endif

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug window");
    ImGui::Combo("Render Mode", reinterpret_cast<int *>(&g_renderMode), g_renderModesStr, static_cast<uint32_t>(eRenderMode::Count));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Camera");
    {
        ImGui::InputFloat("C Near Plane", &g_camera.near);
        ImGui::InputFloat("C Far Plane", &g_camera.far);
        ImGui::InputFloat("C FOV", &g_camera.fov);
        ImGui::Text("C Aspect: %f", g_camera.aspect);
        ImGui::InputFloat3("C Position", g_camera.pos.data, 1);
        ImGui::InputFloat2("C XY Rads", &g_camera.xWorldAngle, 1);
        ImGui::InputFloat("Camera Speed", &g_cameraSpeed);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Pipeline");
    {
        ImGui::NewLine();
        ImGui::Text("Depth pre pass");
        ImGui::InputFloat("Depth Bias", &g_depthBiasScale);
        ImGui::InputFloat("Depth Unit", &g_depthUnitScale);

        ImGui::NewLine();
        ImGui::Text("Ambient Light");
        ImGui::SliderFloat("AL Radiant Flux", &g_ambientLightRadiantFlux, 0, 1);

        ImGui::NewLine();
        static bool enableDirectLightCheckBoxValue = static_cast<bool>(g_directLightEnabled);
        ImGui::Checkbox("Direct Light", &enableDirectLightCheckBoxValue);
        ImGui::SliderFloat("DL Radiant Flux", &g_directLight.radiantFlux, 0, 100);
        g_directLightEnabled = static_cast<bool>(enableDirectLightCheckBoxValue);
        ImGui::InputFloat3("DL Position", g_directLight.position.data);
        ImGui::SliderFloat("DL Rotation X", &g_directLight.orientation.x, -3.14f, 3.14f, "%.5f");
        ImGui::SliderFloat("DL Rotation Y", &g_directLight.orientation.y, -3.14f, 3.14f, "%.5f");
        ImGui::SliderFloat("DL Rotation Z", &g_directLight.orientation.z, -3.14f, 3.14f, "%.5f");
        static bool enableShadowMappingCheckBoxValue = static_cast<bool>(g_shadowMappingEnabled);
        ImGui::Checkbox("Shadow Mapping", &enableShadowMappingCheckBoxValue);
        g_shadowMappingEnabled = static_cast<bool>(enableShadowMappingCheckBoxValue);

        ImGui::NewLine();
        static bool enablePointLightCheckBoxValue = static_cast<bool>(g_pointLightEnabled);
        ImGui::Checkbox("Point Light", &enablePointLightCheckBoxValue);
        g_pointLightEnabled = static_cast<bool>(enablePointLightCheckBoxValue);
        ImGui::SliderFloat("PL Radiant Flux", &g_pointLightRadiantFlux, 0, 100);

        ImGui::NewLine();
        static bool enableBumpMappingCheckboxValue = static_cast<bool>(g_bumpMappingEnabled);
        ImGui::Checkbox("Bump Mapping", &enableBumpMappingCheckboxValue);
        g_bumpMappingEnabled = static_cast<bool>(enableBumpMappingCheckboxValue);
        ImGui::SliderFloat("Bump map scale", &g_bumpMapScaleFactor, 0.0001f, 0.01f, "%.5f");

        ImGui::NewLine();
        ImGui::Checkbox("Draw AABBs", &g_drawAABBs);

        ImGui::NewLine();
        ImGui::Text("Temporal Antialiasing");
        static bool enableTaaCheckboxValue = static_cast<bool>(g_taaEnabled);
        ImGui::Checkbox("TAA", &enableTaaCheckboxValue);
        g_taaEnabled = static_cast<decltype(g_taaEnabled)>(enableTaaCheckboxValue);
        static bool enableTaaJitterCheckboxValue = static_cast<bool>(g_taaJitterEnabled);
        ImGui::Checkbox("Jitter", &enableTaaJitterCheckboxValue);
        g_taaJitterEnabled = static_cast<decltype(g_taaJitterEnabled)>(enableTaaJitterCheckboxValue);

        ImGui::NewLine();
        static bool enabledToneMappingCheckboxValue = static_cast<bool>(g_toneMappingEnabled);
        ImGui::Checkbox("Tone Mapping", &enabledToneMappingCheckboxValue);
        g_toneMappingEnabled = static_cast<decltype(g_toneMappingEnabled)>(enabledToneMappingCheckboxValue);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifdef NDEBUG
    glPopGroupMarkerEXT();
#endif
}

std::vector<RenderModel> LoadOpaqueModels(ShaderProgram const &program)
{
    std::vector<RenderModel> models;
    std::vector<sr::load::Geometry> geometries;
    std::vector<sr::load::MaterialSource> materials;

    sr::load::LoadOBJ("data\\models\\Sponza", "sponza.obj", geometries, materials);
    for (uint32_t i = 0; i < geometries.size(); ++i)
    {
        auto const vertexBufferDescriptors = sr::load::CreateBufferDescriptors(geometries[i]);
        auto const indexBufferDescriptor = sr::load::CreateIndexBufferDescriptor(geometries[i]);

        RenderModelCreateInfo createInfo;
        createInfo.color = {1, 0, 0};
        createInfo.debugRenderModel = 0;
        createInfo.geometry = &geometries[i];
        createInfo.indexBufferDescriptor = &indexBufferDescriptor;
        createInfo.material = &materials[createInfo.geometry->material];
        createInfo.vertexBufferDescriptors = &vertexBufferDescriptors;

        models.push_back(CreateRenderModel(createInfo));
        LinkRenderModelToShaderProgram(
            program.handle, models[i], g_shaderAttributesPositionNormalUV);
    }

    for (auto &material : materials)
    {
        FreeMaterialSource(material);
    }

    return models;
}

std::vector<RenderModel> LoadPointLightModels(ShaderProgram const &program)
{
    std::vector<RenderModel> models;
    std::vector<sr::load::Geometry> geometries;
    std::vector<sr::load::MaterialSource> materials;
    sr::load::LoadOBJ("data\\models\\box", "box.obj", geometries, materials);

    for (uint32_t i = 0; i < g_pointLightCount; ++i)
    {
        auto const vertexBufferDescriptors = sr::load::CreateBufferDescriptors(geometries.back());
        auto const indexBufferDescriptor = sr::load::CreateIndexBufferDescriptor(geometries.back());
        sr::load::MaterialSource const emptyMaterial = {};

        RenderModelCreateInfo createInfo;
        createInfo.color = sr::math::Vec3{1.f, 209 / 255.0f, 163 / 255.0f};
        createInfo.debugRenderModel = 1;
        createInfo.geometry = &geometries.back();
        createInfo.indexBufferDescriptor = &indexBufferDescriptor;
        createInfo.material = &emptyMaterial;
        createInfo.position = g_pointLights[i];
        createInfo.scale = {10, 10, 10};
        createInfo.vertexBufferDescriptors = &vertexBufferDescriptors;

        models.push_back(CreateRenderModel(createInfo));
        LinkRenderModelToShaderProgram(
            program.handle, models.back(), g_shaderAttributesPositionNormalUV);
    }

    for (auto &material : materials)
    {
        FreeMaterialSource(material);
    }

    return models;
}

std::vector<RenderModel> LoadDynamicModels(ShaderProgram const &program)
{
    std::vector<RenderModel> models;
    std::vector<sr::load::Geometry> geometries;
    std::vector<sr::load::MaterialSource> materials;
    sr::load::LoadOBJ("data\\models\\quad", "quad.obj", geometries, materials);

    auto const vertexBufferDescriptors = sr::load::CreateBufferDescriptors(geometries.back());
    auto const indexBufferDescriptor = sr::load::CreateIndexBufferDescriptor(geometries.back());

    if (false)
    {
        tinyobj::material_t material;
        material.diffuse_texname = "Brick_wall_006_COLOR.jpg";
        material.bump_texname = "Brick_wall_006_DISP.jpg";
        material.normal_texname = "Brick_wall_006_NRM.jpg";
        material.roughness_texname = "Brick_wall_006_SPEC.jpg";
        material.unknown_parameter["mat"] = "marbel";
        auto const materialSource = sr::load::CreateMaterialSource(
            "data\\materials\\custom\\brick_wall_006", material);

        RenderModelCreateInfo createInfo;
        createInfo.color = {1, 0, 0};
        createInfo.debugRenderModel = 0;
        createInfo.geometry = &geometries.back();
        createInfo.indexBufferDescriptor = &indexBufferDescriptor;
        createInfo.material = &materialSource;
        createInfo.position = {300, 50, 0};
        createInfo.orientation = {0, 6.28f * 0.65f, 0};
        createInfo.scale = {100, 100, 100};
        createInfo.vertexBufferDescriptors = &vertexBufferDescriptors;

        models.push_back(CreateRenderModel(createInfo));
        LinkRenderModelToShaderProgram(
            program.handle, models.back(), g_shaderAttributesPositionNormalUV);
    }

    {
        tinyobj::material_t material;
        material.diffuse_texname = "shfsaida_2K_Albedo.jpg";
        material.bump_texname = "shfsaida_2K_Bump.jpg";
        material.normal_texname = "shfsaida_2K_Normal.jpg";
        material.roughness_texname = "shfsaida_2K_Roughness.jpg";
        material.unknown_parameter["mat"] = "marbel";
        auto const materialSource = sr::load::CreateMaterialSource(
            "data\\materials\\2k\\Rock_Cliffs_shfsaida_2K_surface_ms", material);

        RenderModelCreateInfo createInfo;
        createInfo.color = {1, 0, 0};
        createInfo.debugRenderModel = 0;
        createInfo.geometry = &geometries.back();
        createInfo.indexBufferDescriptor = &indexBufferDescriptor;
        createInfo.material = &materialSource;
        createInfo.position = {0, 50, 0};
        createInfo.orientation = {0, 6.28f * 0.65f, 0};
        createInfo.scale = {100, 100, 100};
        createInfo.vertexBufferDescriptors = &vertexBufferDescriptors;

        models.push_back(CreateRenderModel(createInfo));
        LinkRenderModelToShaderProgram(
            program.handle, models.back(), g_shaderAttributesPositionNormalUV);
    }

    for (auto &material : materials)
    {
        FreeMaterialSource(material);
    }

    return models;
}

std::vector<RenderModel> LoadAABBModels(ShaderProgram const &program, std::vector<RenderModel> const &inputModels)
{
    std::vector<RenderModel> models;
    std::vector<sr::load::Geometry> geometries;
    std::vector<sr::load::MaterialSource> materials;
    sr::load::LoadOBJ("data\\models\\box", "box.obj", geometries, materials);

    size_t const size = inputModels.size();
    for (auto const &model : inputModels)
    {
        auto const vertexBufferDescriptors = sr::load::CreateBufferDescriptors(geometries.back());
        auto const indexBufferDescriptor = sr::load::CreateIndexBufferDescriptor(geometries.back());
        sr::load::MaterialSource const emptyMaterial = {};

        RenderModelCreateInfo createInfo;
        createInfo.color = sr::math::Vec3{1.f, 0, 0};
        createInfo.debugRenderModel = 1;
        createInfo.geometry = &geometries.back();
        createInfo.indexBufferDescriptor = &indexBufferDescriptor;
        createInfo.material = &emptyMaterial;
        createInfo.position = (model.aabb.max + model.aabb.min) / 2.f;
        createInfo.scale = model.aabb.max - createInfo.position;
        createInfo.vertexBufferDescriptors = &vertexBufferDescriptors;

        models.push_back(CreateRenderModel(createInfo));
        LinkRenderModelToShaderProgram(
            program.handle, models.back(), g_shaderAttributesPositionNormalUV);
    }

    for (auto &material : materials)
    {
        FreeMaterialSource(material);
    }

    return models;
}

ForwardPipelineShaderPrograms CreateForwardPipelineShaderPrograms()
{
    ForwardPipelineShaderPrograms desc;

    desc.depthPrePass = CreateShaderProgram("shaders/depth_pre_pass.vert", "shaders/depth_pre_pass.frag");
    desc.shadowMapping = CreateShaderProgram("shaders/shadow_mapping.vert", "shaders/shadow_mapping.frag");
    desc.lighting = CreateShaderProgram("shaders/lighting.vert", "shaders/lighting.frag");
    desc.transparent = CreateShaderProgram("shaders/lighting.vert", "shaders/lighting.frag");
    desc.velocity = CreateShaderProgram("shaders/velocity.vert", "shaders/velocity.frag");
    desc.taa = CreateShaderProgram("shaders/taa.vert", "shaders/taa.frag");
    LinkRenderModelToShaderProgram(desc.taa.handle, g_quadWallRenderModel, g_shaderAttributesPositionNormalUV);
    desc.toneMapping = CreateShaderProgram("shaders/tone_mapping.vert", "shaders/tone_mapping.frag");
    LinkRenderModelToShaderProgram(desc.lighting.handle, g_quadWallRenderModel, g_shaderAttributesPositionNormalUV);
    desc.debug = CreateShaderProgram("shaders/debug.vert", "shaders/debug.frag");
    LinkRenderModelToShaderProgram(desc.debug.handle, g_quadWallRenderModel, g_shaderAttributesPositionNormalUV);

    return desc;
}

void CreateForwardPipelineUniformBindngs(ForwardPipelineShaderPrograms &desc,
                                         std::vector<RenderModel> const &opaqueModels,
                                         std::vector<RenderModel> const &transparentModels)
{
    {
        CreateShaderProgramUniformBindings(
            desc.depthPrePass,
            UniformsDescriptor{
                UniformsDescriptor::PerFrameUI32{
                    {"uTaaEnabledUint", "uTaaJitterEnabledUint"},
                    {&g_taaEnabled, &g_taaJitterEnabled},
                    {1, 1}},
                UniformsDescriptor::PerFrameFloat1{},
                UniformsDescriptor::PerFrameFloat2{},
                UniformsDescriptor::PerFrameFloat3{},
                UniformsDescriptor::PerFrameFloat4{},
                UniformsDescriptor::PerFrameMat4{
                    {"uProjMat", "uProjUnjitMat", "uViewMat"},
                    {g_camera.proj.data, g_taaBuffer.projUnjit.data, g_camera.view.data},
                    {1, 1, 1}},
                UniformsDescriptor::PerModelUI32{},
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                UniformsDescriptor::PerModelFloat3{},
                UniformsDescriptor::PerModelFloat4{},
                UniformsDescriptor::PerModelMat4{
                    {"uModelMat"},
                    {reinterpret_cast<float const *>(opaqueModels.data())},
                    {offsetof(RenderModel, RenderModel::model)},
                    {sizeof(RenderModel)}},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.shadowMapping,
            UniformsDescriptor{
                UniformsDescriptor::PerFrameUI32{},
                UniformsDescriptor::PerFrameFloat1{},
                UniformsDescriptor::PerFrameFloat2{},
                UniformsDescriptor::PerFrameFloat3{},
                UniformsDescriptor::PerFrameFloat4{},
                UniformsDescriptor::PerFrameMat4{
                    {"uProjMat", "uViewMat"},
                    {g_directLight.projection.data, g_directLight.view.data},
                    {1, 1}},
                UniformsDescriptor::PerModelUI32{},
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                UniformsDescriptor::PerModelFloat3{},
                UniformsDescriptor::PerModelFloat4{},
                UniformsDescriptor::PerModelMat4{
                    {"uModelMat"},
                    {reinterpret_cast<float const *>(opaqueModels.data())},
                    {offsetof(RenderModel, RenderModel::model)},
                    {sizeof(RenderModel)}},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.lighting,
            UniformsDescriptor{
                //uint32_t
                UniformsDescriptor::PerFrameUI32{
                    {"uRenderModeUint",
                     "uDirectLightEnabledUint",
                     "uPointLightEnabledUint",
                     "uShadowMappingEnabledUint",
                     "uBumpMappingEnabledUint",
                     "uTaaEnabledUint",
                     "uTaaJitterEnabledUint"},
                    {reinterpret_cast<uint32_t *>(&g_renderMode),
                     &g_directLightEnabled,
                     &g_pointLightEnabled,
                     &g_shadowMappingEnabled,
                     &g_bumpMappingEnabled,
                     &g_taaEnabled,
                     &g_taaJitterEnabled},
                    {1, 1, 1, 1, 1, 1, 1}},
                //floats
                UniformsDescriptor::PerFrameFloat1{
                    {"uBumpMapScaleFactorFloat", "uAmbientLightRadiantFluxFloat", "uDirectLightRadiantFluxFloat", "uPointLightRadiantFluxFloat"},
                    {&g_bumpMapScaleFactor, &g_ambientLightRadiantFlux, &g_directLight.radiantFlux, &g_pointLightRadiantFlux},
                    {1, 1, 1, 1}},
                UniformsDescriptor::PerFrameFloat2{},
                //float3
                UniformsDescriptor::PerFrameFloat3{
                    {"uCameraPos", "uPointLightPosVec3Array"},
                    {g_camera.pos.data,
                     g_pointLights[0].data},
                    {1, g_pointLightCount}},
                UniformsDescriptor::PerFrameFloat4{},
                //mat4
                UniformsDescriptor::PerFrameMat4{
                    {"uProjMat",
                     "uProjUnjitMat",
                     "uViewMat",
                     "uDirLightProjMat",
                     "uDirLightViewMat"},
                    {g_camera.proj.data,
                     g_taaBuffer.projUnjit.data,
                     g_camera.view.data,
                     g_directLight.projection.data,
                     g_directLight.view.data},
                    {1, 1, 1, 1, 1}},
                //uint32_t array
                UniformsDescriptor::PerModelUI32{
                    {"uBumpMapAvailableUint",
                     "uMetallicMapAvailableUint",
                     "uRoughnessMapAvailableUint",
                     "uDebugRenderModeEnabledUint",
                     "uBrdfUint"},
                    {reinterpret_cast<uint32_t const *>(opaqueModels.data()),
                     reinterpret_cast<uint32_t const *>(opaqueModels.data()),
                     reinterpret_cast<uint32_t const *>(opaqueModels.data()),
                     reinterpret_cast<uint32_t const *>(opaqueModels.data()),
                     reinterpret_cast<uint32_t const *>(opaqueModels.data())},
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
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                //float3
                UniformsDescriptor::PerModelFloat3{
                    {"uColor"},
                    {reinterpret_cast<float const *>(opaqueModels.data())},
                    {offsetof(RenderModel, RenderModel::color)},
                    {sizeof(RenderModel)}},
                UniformsDescriptor::PerModelFloat4{},
                //mat4
                UniformsDescriptor::PerModelMat4{
                    {"uModelMat"},
                    {reinterpret_cast<float const *>(opaqueModels.data())},
                    {offsetof(RenderModel, RenderModel::model)},
                    {sizeof(RenderModel)}},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.transparent,
            UniformsDescriptor{
                //uint32_t
                UniformsDescriptor::PerFrameUI32{
                    {"uRenderModeUint",
                     "uDirectLightEnabledUint",
                     "uPointLightEnabledUint",
                     "uShadowMappingEnabledUint",
                     "uBumpMappingEnabledUint",
                     "uTaaEnabledUint",
                     "uTaaJitterEnabledUint"},
                    {reinterpret_cast<uint32_t *>(&g_renderMode),
                     &g_directLightEnabled,
                     &g_pointLightEnabled,
                     &g_shadowMappingEnabled,
                     &g_bumpMappingEnabled,
                     &g_taaEnabled,
                     &g_taaJitterEnabled},
                    {1, 1, 1, 1, 1, 1, 1}},
                //floats
                UniformsDescriptor::PerFrameFloat1{
                    {"uBumpMapScaleFactorFloat", "uAmbientLightRadiantFluxFloat", "uDirectLightRadiantFluxFloat", "uPointLightRadiantFluxFloat"},
                    {&g_bumpMapScaleFactor, &g_ambientLightRadiantFlux, &g_directLight.radiantFlux, &g_pointLightRadiantFlux},
                    {1, 1, 1, 1}},
                UniformsDescriptor::PerFrameFloat2{},
                //float3
                UniformsDescriptor::PerFrameFloat3{
                    {"uCameraPos", "uPointLightPosVec3Array"},
                    {g_camera.pos.data,
                     g_pointLights[0].data},
                    {1, g_pointLightCount}},
                UniformsDescriptor::PerFrameFloat4{},
                //mat4
                UniformsDescriptor::PerFrameMat4{
                    {"uProjMat",
                     "uProjUnjitMat",
                     "uViewMat",
                     "uDirLightProjMat",
                     "uDirLightViewMat"},
                    {g_camera.proj.data,
                     g_taaBuffer.projUnjit.data,
                     g_camera.view.data,
                     g_directLight.projection.data,
                     g_directLight.view.data},
                    {1, 1, 1, 1, 1}},
                //uint32_t array
                UniformsDescriptor::PerModelUI32{
                    {"uBumpMapAvailableUint",
                     "uMetallicMapAvailableUint",
                     "uRoughnessMapAvailableUint",
                     "uDebugRenderModeEnabledUint",
                     "uBrdfUint"},
                    {reinterpret_cast<uint32_t const *>(transparentModels.data()),
                     reinterpret_cast<uint32_t const *>(transparentModels.data()),
                     reinterpret_cast<uint32_t const *>(transparentModels.data()),
                     reinterpret_cast<uint32_t const *>(transparentModels.data()),
                     reinterpret_cast<uint32_t const *>(transparentModels.data())},
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
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                //float3
                UniformsDescriptor::PerModelFloat3{
                    {"uColor"},
                    {reinterpret_cast<float const *>(transparentModels.data())},
                    {offsetof(RenderModel, RenderModel::color)},
                    {sizeof(RenderModel)}},
                UniformsDescriptor::PerModelFloat4{},
                //mat4
                UniformsDescriptor::PerModelMat4{
                    {"uModelMat"},
                    {reinterpret_cast<float const *>(transparentModels.data())},
                    {offsetof(RenderModel, RenderModel::model)},
                    {sizeof(RenderModel)}},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.velocity,
            UniformsDescriptor{
                UniformsDescriptor::PerFrameUI32{},
                UniformsDescriptor::PerFrameFloat1{},
                UniformsDescriptor::PerFrameFloat2{},
                UniformsDescriptor::PerFrameFloat3{},
                UniformsDescriptor::PerFrameFloat4{},
                UniformsDescriptor::PerFrameMat4{
                    {"uPrevViewMat4",
                     "uPrevProjUnjitMat4",
                     "uViewMat4",
                     "uProjUnjitMat4"},
                    {g_prevCamera.view.data,
                     g_taaBuffer.prevProjUnjit.data,
                     g_camera.view.data,
                     g_taaBuffer.projUnjit.data},
                    {1, 1, 1, 1},
                },
                UniformsDescriptor::PerModelUI32{},
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                UniformsDescriptor::PerModelFloat3{},
                UniformsDescriptor::PerModelFloat4{},
                UniformsDescriptor::PerModelMat4{
                    {"uPrevModelMat4", "uModelMat4"},
                    {reinterpret_cast<float const *>(g_taaBuffer.prevModels.data()),
                     reinterpret_cast<float const *>(opaqueModels.data())},
                    {0, offsetof(RenderModel, RenderModel::model)},
                    {sizeof(sr::math::Matrix4x4), sizeof(RenderModel)},
                },
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.taa,
            UniformsDescriptor{
                UniformsDescriptor::PerFrameUI32{
                    {"uFrameCountUint",
                     "uTaaEnabledUint",
                     "uTaaJitterEnabledUint"},
                    {&g_taaBuffer.count,
                     &g_taaEnabled,
                     &g_taaJitterEnabled},
                    {1, 1, 1}},
                UniformsDescriptor::PerFrameFloat1{},
                UniformsDescriptor::PerFrameFloat2{
                    {"uJitterVec2"}, {g_taaBuffer.jitter.data}, {1}},
                UniformsDescriptor::PerFrameFloat3{},
                UniformsDescriptor::PerFrameFloat4{},
                UniformsDescriptor::PerFrameMat4{
                    {"uViewMat",
                     "uProjMat",
                     "uProjUnjitMat",
                     "uPrevViewMat",
                     "uPrevProjMat",
                     "uPrevProjUnjitMat"},
                    {g_camera.view.data,
                     g_camera.proj.data,
                     g_taaBuffer.projUnjit.data,
                     g_prevCamera.view.data,
                     g_prevCamera.proj.data,
                     g_taaBuffer.prevProjUnjit.data},
                    {1, 1, 1, 1, 1, 1}},
                UniformsDescriptor::PerModelUI32{},
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                UniformsDescriptor::PerModelFloat3{},
                UniformsDescriptor::PerModelFloat4{},
                UniformsDescriptor::PerModelMat4{},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.toneMapping,
            UniformsDescriptor{
                UniformsDescriptor::PerFrameUI32{
                    {"uToneMappingEnabledUint"}, {&g_toneMappingEnabled}, {1}},
                UniformsDescriptor::PerFrameFloat1{},
                UniformsDescriptor::PerFrameFloat2{},
                UniformsDescriptor::PerFrameFloat3{},
                UniformsDescriptor::PerFrameFloat4{},
                UniformsDescriptor::PerFrameMat4{},
                UniformsDescriptor::PerModelUI32{},
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                UniformsDescriptor::PerModelFloat3{},
                UniformsDescriptor::PerModelFloat4{},
                UniformsDescriptor::PerModelMat4{},
            });
    }

    {
        CreateShaderProgramUniformBindings(
            desc.debug,
            UniformsDescriptor{
                UniformsDescriptor::PerFrameUI32{
                    {"uTaaEnabledUint"}, {&g_taaEnabled}, {1}},
                UniformsDescriptor::PerFrameFloat1{},
                UniformsDescriptor::PerFrameFloat2{},
                UniformsDescriptor::PerFrameFloat3{},
                UniformsDescriptor::PerFrameFloat4{},
                UniformsDescriptor::PerFrameMat4{},
                UniformsDescriptor::PerModelUI32{},
                UniformsDescriptor::PerModelFloat1{},
                UniformsDescriptor::PerModelFloat2{},
                UniformsDescriptor::PerModelFloat3{},
                UniformsDescriptor::PerModelFloat4{},
                UniformsDescriptor::PerModelMat4{},
            });
    }
}

void UpdateInputs(GLFWwindow *window)
{
    assert(window != nullptr);

    sr::input::UpdateInputs();

    sr::math::Vec4 forward = {};
    sr::math::Vec4 right = {};
    sr::math::Matrix4x4 const camera = CreateCameraMatrix(
        g_camera.pos, g_camera.xWorldAngle, g_camera.yWorldAngle);

    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::W)] != sr::input::KeyAction::NONE)
    {
        forward = sr::math::Mul(camera, sr::math::Vec4{0, 0, -1, 0});
        forward *= g_cameraSpeed;
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::S)] != sr::input::KeyAction::NONE)
    {
        forward = sr::math::Mul(camera, sr::math::Vec4{0, 0, -1, 0});
        forward *= -g_cameraSpeed;
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::A)] != sr::input::KeyAction::NONE)
    {
        right = sr::math::Mul(camera, sr::math::Vec4{-1, 0, 0, 0});
        right *= g_cameraSpeed;
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::D)] != sr::input::KeyAction::NONE)
    {
        right = sr::math::Mul(camera, sr::math::Vec4{-1, 0, 0, 0});
        right *= -g_cameraSpeed;
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::Q)] != sr::input::KeyAction::NONE)
    {
        g_camera.pos.y -= g_cameraSpeed;
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::E)] != sr::input::KeyAction::NONE)
    {
        g_camera.pos.y += g_cameraSpeed;
    }

    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::F)] == sr::input::KeyAction::RELEASE)
    {
        g_captureMouse = !g_captureMouse;
        glfwSetInputMode(window, GLFW_CURSOR, g_captureMouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::F3)] == sr::input::KeyAction::RELEASE)
    {
        g_renderMode = g_renderMode == eRenderMode::Full ? g_renderMode = eRenderMode::Count : g_renderMode;
        g_renderMode = static_cast<eRenderMode>(static_cast<uint32_t>(g_renderMode) - 1);
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::F4)] == sr::input::KeyAction::RELEASE)
    {
        g_renderMode = static_cast<eRenderMode>(static_cast<uint32_t>(g_renderMode) + 1);
        g_renderMode = g_renderMode == eRenderMode::Count ? g_renderMode = eRenderMode::Full : g_renderMode;
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::F5)] == sr::input::KeyAction::RELEASE)
    {
        printf("\033c");
        g_isHotRealoadRequired = true;
    }
    if (sr::input::g_inputs.keys[static_cast<uint32_t>(sr::input::Keys::U)] == sr::input::KeyAction::RELEASE)
    {
        g_drawUi = !g_drawUi;
    }

    g_camera.pos += forward.xyz;
    g_camera.pos += right.xyz;
}

void UpdateModels(std::vector<RenderModel> &models)
{
    models.back().model = sr::math::CreateRotationMatrixY(0.01f) * models.back().model;
}

void PrePassCommands(ForwardPipeline &pipeline, std::vector<RenderModel> &models)
{
    { // Update shadow map view frustum
        g_directLight.view = sr::math::CreateRotationMatrixZ(-g_directLight.orientation.z) *
                             sr::math::CreateRotationMatrixY(-g_directLight.orientation.y) *
                             sr::math::CreateRotationMatrixX(-g_directLight.orientation.x) *
                             sr::math::CreateTranslationMatrix(-g_directLight.position);

        sr::math::Vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
        sr::math::Vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (auto const &model : models)
        {
            auto const aabb = sr::geo::AABB{
                (g_directLight.view * sr::math::Vec4{model.aabb.min.x, model.aabb.min.y, model.aabb.min.z, 1}).xyz,
                (g_directLight.view * sr::math::Vec4{model.aabb.max.x, model.aabb.max.y, model.aabb.max.z, 1}).xyz,
            };

            min.x = aabb.min.x < min.x ? aabb.min.x : min.x;
            min.y = aabb.min.y < min.y ? aabb.min.y : min.y;
            min.z = aabb.min.z < min.z ? aabb.min.z : min.z;
            min.x = aabb.max.x < min.x ? aabb.max.x : min.x;
            min.y = aabb.max.y < min.y ? aabb.max.y : min.y;
            min.z = aabb.max.z < min.z ? aabb.max.z : min.z;

            max.x = aabb.max.x > max.x ? aabb.max.x : max.x;
            max.y = aabb.max.y > max.y ? aabb.max.y : max.y;
            max.z = aabb.max.z > max.z ? aabb.max.z : max.z;
            max.x = aabb.min.x > max.x ? aabb.min.x : max.x;
            max.y = aabb.min.y > max.y ? aabb.min.y : max.y;
            max.z = aabb.min.z > max.z ? aabb.min.z : max.z;
        }

        assert(min.x != max.x);
        assert(min.y != max.y);
        assert(min.z != max.z);

        g_directLight.frustum.far = min.z;
        g_directLight.frustum.near = max.z;
        g_directLight.frustum.left = max.x;
        g_directLight.frustum.right = min.x;
        g_directLight.frustum.top = max.y;
        g_directLight.frustum.bottom = min.y;

        g_directLight.projection = sr::math::CreateOrthographicProjectionMatrix(
            g_directLight.frustum.left,
            g_directLight.frustum.right,
            g_directLight.frustum.bottom,
            g_directLight.frustum.top,
            g_directLight.frustum.near,
            g_directLight.frustum.far);
    }

    { // Save previous frame model matrices
        g_taaBuffer.prevModels.resize(models.size());
        for (uint64_t i = 0; i < models.size(); ++i)
        {
            g_taaBuffer.prevModels[i] = models[i].model;
        }
    }

    UpdateModels(models);

    { //Save previous frame camera materices
        g_prevCamera = g_camera;
        g_taaBuffer.prevProjUnjit = g_taaBuffer.projUnjit;
    }

    { // Update view projection with respect to TAA
        g_camera.view = CreateViewMatrix(g_camera.pos, g_camera.xWorldAngle, g_camera.yWorldAngle);
        // g_camera.proj = sr::math::CreateOrthographicProjectionMatrix(
        //     -2048.f, 2048.f, -2048.f, 2048.f, -2000.f, 1500.f);
        const uint32_t taaSampleIndex = g_taaBuffer.count % 16;

        // https://github.com/playdeadgames/temporal
        float const oneExtentY = std::tan(0.5f * g_camera.fov);
        float const oneExtentX = oneExtentY * g_camera.aspect;
        float const texelSizeX = oneExtentX / (0.5f * pipeline.debug.width);
        float const texelSizeY = oneExtentY / (0.5f * pipeline.debug.height);
        float const oneJitterX = texelSizeX * g_taaHalton23Sequence16[taaSampleIndex].x;
        float const oneJitterY = texelSizeY * g_taaHalton23Sequence16[taaSampleIndex].y;

        //Note: Below is Inside approach, might be better that mine
        // float const cf = g_camera.far;
        // float const cn = g_camera.near;
        // float const xm = oneJitterX - oneExtentX;
        // float const xp = oneJitterX + oneExtentX;
        // float const ym = oneJitterY - oneExtentY;
        // float const yp = oneJitterY + oneExtentY;
        //g_camera.proj = sr::math::CreatePerspectiveProjectionMatrix(
        //    xm * cn, xp * cn, ym * cn, yp * cn, cn, cf);
        g_taaBuffer.jitter = {oneJitterX, oneJitterY};
        g_taaBuffer.projUnjit = sr::math::CreatePerspectiveProjectionMatrix(
            g_camera.near, g_camera.far, g_camera.fov, g_camera.aspect);
        g_camera.proj = sr::math::Mul(
            sr::math::CreateTranslationMatrix(oneJitterX, oneJitterY, 0), g_taaBuffer.projUnjit);
    }
}

void RenderPassDepthPrePass(ForwardPipeline &pipeline, std::vector<RenderModel> const &models)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(g_depthBiasScale, g_depthUnitScale);

    ExecuteRenderPass(pipeline.depthPrePass, models.data(), models.size());

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void RenderPassTAA(ForwardPipeline &pipeline)
{
    ExecuteRenderPass(pipeline.taa, &g_quadWallRenderModel, 1);

    pipeline.taa.subPasses[0].active = !pipeline.taa.subPasses[0].active;
    pipeline.taa.subPasses[1].active = !pipeline.taa.subPasses[0].active;

    g_taaBuffer.count++;
}

void RenderPassToneMapping(ForwardPipeline &pipeline)
{
    ExecuteRenderPass(pipeline.toneMapping, &g_quadWallRenderModel, 1);

    pipeline.toneMapping.subPasses[0].desc.dependencies[0].handle =
        pipeline.taa.subPasses[0].active
            ? pipeline.taa.subPasses[0].desc.attachments[0].handle
            : pipeline.taa.subPasses[1].desc.attachments[0].handle;
}

void RenderPassDebug(ForwardPipeline &pipeline)
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

    auto programs = CreateForwardPipelineShaderPrograms();
    auto opaqueModels = LoadOpaqueModels(programs.lighting);
    auto transparentModels = LoadAABBModels(programs.transparent, opaqueModels);
    auto pointLightModels = LoadPointLightModels(programs.lighting);
    opaqueModels.insert(opaqueModels.end(), pointLightModels.begin(), pointLightModels.end());
    std::vector<RenderModel>().swap(pointLightModels);
    auto dynamicModels = LoadDynamicModels(programs.lighting);
    opaqueModels.insert(opaqueModels.end(), dynamicModels.begin(), dynamicModels.end());
    std::vector<RenderModel>().swap(dynamicModels);

    g_taaBuffer.prevModels.resize(opaqueModels.size());
    CreateForwardPipelineUniformBindngs(programs, opaqueModels, transparentModels);
    auto forwardPipeline = CreateForwardRenderPipeline(programs, swapchainFramebufferWidth, swapchainFramebufferHeight);

    while (!glfwWindowShouldClose(window))
    {
        UpdateInputs(window);
        glfwGetFramebufferSize(window, &swapchainFramebufferWidth, &swapchainFramebufferHeight);

        if (g_isHotRealoadRequired)
        {
            DeleteRenderPipeline(forwardPipeline.passes, ForwardPipelinePassCount);

            programs = CreateForwardPipelineShaderPrograms();
            CreateForwardPipelineUniformBindngs(programs, opaqueModels, transparentModels);
            memcpy(&forwardPipeline,
                &CreateForwardRenderPipeline(programs, swapchainFramebufferWidth, swapchainFramebufferHeight),
                sizeof(ForwardPipeline));
            
            std::time_t const timestamp = std::time(nullptr);
            std::cout << "Backbuffer size: " << swapchainFramebufferWidth << "x" << swapchainFramebufferHeight
                      << "\nHot reload: " << std::asctime(std::localtime(&timestamp)) << std::endl;

            g_isHotRealoadRequired = false;
        }

        PrePassCommands(forwardPipeline, opaqueModels);

        RenderPassDepthPrePass(forwardPipeline, opaqueModels);
        ExecuteRenderPass(forwardPipeline.shadowMapping, opaqueModels.data(), opaqueModels.size());
        ExecuteRenderPass(forwardPipeline.lighting, opaqueModels.data(), opaqueModels.size());
        if (g_drawAABBs)
        {
            ExecuteRenderPass(forwardPipeline.transparent, transparentModels.data(), transparentModels.size());
        }
        ExecuteRenderPass(forwardPipeline.velocity, opaqueModels.data(), opaqueModels.size());
        RenderPassTAA(forwardPipeline);
        RenderPassToneMapping(forwardPipeline);
        RenderPassDebug(forwardPipeline);

        ExecuteBackBufferBlitRenderPass(
            forwardPipeline.debug.subPasses[0].fbo,
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
