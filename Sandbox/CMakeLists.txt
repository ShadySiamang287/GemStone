project(Sandbox LANGUAGES CXX)

add_executable(Sandbox WIN32)

target_sources(Sandbox PRIVATE
    src/SandboxApp.cpp
    src/TestScene.cpp
    src/RenderPasses/DeferredPass.cpp
    src/RenderPasses/LightingPass.cpp
    src/RenderPasses/ToneMappingPass.cpp
    src/Camera.cpp
    src/RenderPasses/AtmospherePass.cpp
)

target_include_directories(Sandbox PRIVATE ${Sandbox_SOURCE_DIR}/include)

target_precompile_headers(Sandbox PRIVATE ${Engine_SOURCE_DIR}/include/pch.h)


target_link_libraries(Sandbox
PRIVATE
    Engine
    imgui
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME TriangleVS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Deferred.hlsl"
    ENTRY vertMain
    PROFILE vs_6_6
    INCLUDE_DIRS
        "${Sandbox_SOURCE_DIR}/Shaders"
    ALL
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME TrianglePS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Deferred.hlsl"
    ENTRY fragMain
    PROFILE ps_6_6
    INCLUDE_DIRS "${Sandbox_SOURCE_DIR}/Shaders"
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME FullscreenVS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/FullScreenVert.hlsl"
    ENTRY FullscreenVS
    PROFILE vs_6_6
    INCLUDE_DIRS "${Sandbox_SOURCE_DIR}/Shaders"
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME ToneMappingPS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/ToneMapping.hlsl"
    ENTRY ToneMappingPS
    PROFILE ps_6_6
    INCLUDE_DIRS "${Sandbox_SOURCE_DIR}/Shaders"
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME LightingPS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Lighting.hlsl"
    ENTRY LightingPS
    PROFILE ps_6_6
    INCLUDE_DIRS "${Sandbox_SOURCE_DIR}/Shaders"
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME PlanetHS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/planetHullShader.hlsl"
    ENTRY PlanetHullShader
    PROFILE hs_6_6
    INCLUDE_DIRS "${Sandbox_SOURCE_DIR}/Shaders"
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME PlanetDS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/PlanetDomain.hlsl"
    ENTRY PlanetDS
    PROFILE ds_6_6
    INCLUDE_DIRS "${Sandbox_SOURCE_DIR}/Shaders"
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME AtmosphereVS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Atmosphere.hlsl"
    ENTRY atmosphereVS
    PROFILE vs_6_6
    INCLUDE_DIRS
        "${Sandbox_SOURCE_DIR}/Shaders"
    ALL
)

add_hlsl_shader(
    TARGET GemStoneShaders
    NAME AtmospherePS
    SOURCE "${Sandbox_SOURCE_DIR}/Shaders/Atmosphere.hlsl"
    ENTRY atmospherePS
    PROFILE ps_6_6
    INCLUDE_DIRS "${Sandbox_SOURCE_DIR}/Shaders"
)

add_dependencies(Sandbox GemStoneShaders)

add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:imgui>
        $<TARGET_FILE_DIR:Sandbox>
)