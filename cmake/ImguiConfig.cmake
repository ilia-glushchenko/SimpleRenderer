# Copyright (C) 2019 by Ilya Glushchenko
# This code is licensed under the MIT license (MIT)
# (http://opensource.org/licenses/MIT)

set(IMGUI_ROOT_DIR "${SIMPLE_RENDERER_ROOT}/third_party/imgui" CACHE STRING "Path to ImGUI root directory")
set(IMGUI_SOURCES
    "${IMGUI_ROOT_DIR}/imgui.cpp"
    "${IMGUI_ROOT_DIR}/imgui_widgets.cpp"
    "${IMGUI_ROOT_DIR}/imgui_draw.cpp"
    "${IMGUI_ROOT_DIR}/imgui_demo.cpp"
    "${IMGUI_ROOT_DIR}/examples/imgui_impl_glfw.cpp"
    "${IMGUI_ROOT_DIR}/examples/imgui_impl_opengl3.cpp"
)
set(IMGUI_INCLUDE_DIRS
    "${IMGUI_ROOT_DIR}"
    "${IMGUI_ROOT_DIR}/examples"
    CACHE STRING "Path to ImGUI include directories")
