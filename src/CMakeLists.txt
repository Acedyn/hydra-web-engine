# Cmake version 3.0 or newer required
cmake_minimum_required(VERSION 3.0)

# Create compile command json file for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Hydra-Engine
    VERSION 1.0
    LANGUAGES CXX
    DESCRIPTION "Hydra delegate engine running on WebGPU")

set(CMAKE_EXECUTABLE_SUFFIX ".html")

add_subdirectory(engine)
