# Copyright (C) 2019 by Ilya Glushchenko
# This code is licensed under the MIT license (MIT)
# (http://opensource.org/licenses/MIT)

cmake_minimum_required (VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(SimpleRendererConfig)

project(${SIMPLE_RENDERER_NAME})
set(CMAKE_CXX_STANDARD 17)

#GLFW
include(GlfwConfig)
add_subdirectory(${GLFW_SOURCE_DIR})
include_directories(${GLFW_INCLUDE_DIR})

#GLBINDING
include(GlbindingConfig)
list(APPEND CMAKE_MODULE_PATH "${GLBINDING_SOURCE_DIR}/third_party/glbinding/cmake")
add_subdirectory(${GLBINDING_SOURCE_DIR})
include_directories(${GLBINDING_INCLUDE_DIR})

#STB
include(StbConfig)
include_directories(${STB_INCLUDE_DIR})

#TinyOBJLoader
include(TinyObjLoaderConfig)
add_subdirectory(${TINY_OBJ_LOADER_ROOT_DIR})
include_directories(${TINY_OBJ_LOADER_INCLUDE_DIR})

#ImGui
include(ImguiConfig)
include_directories(${IMGUI_INCLUDE_DIRS})

#Target
include_directories(
    ${SIMPLE_RENDERER_INCLUDE_DIR}
)
set(SIMPLE_REDNERER_HEADERS
    include/RenderConfiguration.hpp
    include/RenderDefinitions.hpp
    include/RenderPass.hpp
    include/RenderModel.hpp
    include/ShaderProgram.hpp
    include/Camera.hpp
    include/TestModels.hpp
    include/Texture.hpp
    include/Math.hpp
    include/Loader.hpp
)

set(SIMPLE_RENDERER_SOURCES
    source/Main.cpp
)

add_executable (${PROJECT_NAME}
    ${SIMPLE_REDNERER_HEADERS}
    ${SIMPLE_RENDERER_SOURCES}
    ${IMGUI_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    ${GLBINDING_LIB}
    ${GLFW_LIB}
)
