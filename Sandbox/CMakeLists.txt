project(Sandbox LANGUAGES CXX)

add_executable(Sandbox WIN32)

target_sources(Sandbox PRIVATE
    src/SandboxApp.cpp
)

target_include_directories(Sandbox PRIVATE ${Sandbox_SOURCE_DIR}/include)

target_precompile_headers(Sandbox PRIVATE ${Engine_SOURCE_DIR}/include/pch.h)


target_link_libraries(Sandbox
PRIVATE
    Engine
)

add_slang_shader(
    TARGET GemStoneShaders
    NAME TriangleVS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Triangle.slang"
    ENTRY vertMain
    PROFILE vs_6_6
    INCLUDE_DIRS
        "${Sandbox_SOURCE_DIR}/Shaders"
    ALL
)

add_slang_shader(
    TARGET GemStoneShaders
    NAME TrianglePS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Triangle.slang"
    ENTRY fragMain
    PROFILE ps_6_6
    INCLUDE_DIRS
        "${Sandbox_SOURCE_DIR}/Shaders"
)

add_slang_shader(
    TARGET GemStoneShaders
    NAME FullscreenVS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/FullScreenVert.slang"
    ENTRY FullscreenVS
    PROFILE vs_6_6
    INCLUDE_DIRS
        "${Sandbox_SOURCE_DIR}/Shaders"
    ALL
)

add_slang_shader(
    TARGET GemStoneShaders
    NAME ToneMappingPS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/ToneMapping.slang"
    ENTRY ToneMappingPS
    PROFILE ps_6_6
    INCLUDE_DIRS
        "${Sandbox_SOURCE_DIR}/Shaders"
)

add_slang_shader(
    TARGET GemStoneShaders
    NAME LightingPS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Lighting.slang"
    ENTRY LightingPS
    PROFILE ps_6_6
    INCLUDE_DIRS
        "${Sandbox_SOURCE_DIR}/Shaders"
)


add_dependencies(Sandbox GemStoneShaders)