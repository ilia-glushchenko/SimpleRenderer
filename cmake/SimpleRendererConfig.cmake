# Copyright (C) 2019 by Godlike
# This code is licensed under the MIT license (MIT)
# (http://opensource.org/licenses/MIT)

set(SIMPLE_RENDERER_NAME "SimpleRenderer" CACHE STRING "Project name.")

set(CMAKE_CXX_STANDARD 17)

if(NOT DEFINED SIMPLE_RENDERER_ROOT)
    set(SIMPLE_RENDERER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "Root directory.")
endif()

set(SIMPLE_RENDERER_INCLUDE_DIR
    ${SIMPLE_RENDERER_ROOT}
    ${SIMPLE_RENDERER_ROOT}/include
    CACHE LIST "Include directories."
)
