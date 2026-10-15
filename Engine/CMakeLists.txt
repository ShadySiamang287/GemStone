project(Engine LANGUAGES CXX)

add_library(Engine SHARED)

target_compile_definitions(Engine
PUBLIC 
    GEM_PLATFORM_WINDOWS
PRIVATE
    GEM_BUILD_DLL
    UNICODE
    _UNICODE
)

target_precompile_headers(Engine PRIVATE ${Engine_SOURCE_DIR}/include/pch.h)

target_sources(Engine PRIVATE
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/Graphics/GraphicsContext.cpp
    src/Graphics/GraphicsCommands.cpp
    src/Graphics/GraphicsPipeline.cpp
    src/Graphics/ResourceFactory.cpp
    src/Renderer/RenderTargetManager.cpp
    src/Renderer/Renderer.cpp
    src/Renderer/RenderPasses/DeferredPass.cpp
    src/Renderer/RenderPasses/LightingPass.cpp
    src/Renderer/RenderPasses/ToneMappingPass.cpp
)

target_include_directories(Engine
    PUBLIC ${Engine_SOURCE_DIR}/include
)

target_compile_features(Engine 
PUBLIC 
    cxx_std_23
)

target_link_libraries(Engine
PUBLIC
#FetchContent dependencies
    DirectXMath
    DirectX-Headers
    D3D12MemoryAllocator
PRIVATE
    #directX
    d3d12
    dxgi
    dxguid
    WinPixEventRuntime

)

add_custom_command(TARGET Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Engine>
        $<TARGET_FILE_DIR:Sandbox>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:WinPixEventRuntime>
        $<TARGET_FILE_DIR:Sandbox>
)
