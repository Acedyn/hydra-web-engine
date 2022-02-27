# Hydra Engine is an hydra delegate WebGPU rendering engine
add_executable(hydra-engine
    main.cpp)

if(EMSCRIPTEN)
    target_include_directories(hydra-engine
        PUBLIC ${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include)
endif()
