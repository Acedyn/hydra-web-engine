# Cmake version 3.0 or newer required
cmake_minimum_required(VERSION 3.0)

# Create compile command json file for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Hydra-Engine
    VERSION 1.0
    LANGUAGES CXX
    DESCRIPTION "Hydra delegate engine running on WebGPU")

set(EMSCRIPTEN_COMPILE_FLAGS "-s MODULARIZE=1 -s EXPORTED_RUNTIME_METHODS=ccall, USE_WEBGPU=1 NO_EXIT_RUNTIME=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_COMPILE_FLAGS}")
set(CMAKE_EXECUTABLE_SUFFIX ".js")

add_subdirectory(engine)
