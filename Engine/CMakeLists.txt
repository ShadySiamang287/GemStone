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
)

target_include_directories(Engine
    PUBLIC ${Engine_SOURCE_DIR}/include
)

target_compile_features(Engine 
PUBLIC 
    cxx_std_23
)

target_link_libraries(Engine PRIVATE 
    #directX
    d3d12
    dxgi
    dxguid

    #FetchContent dependencies
    DirectXMath
    DirectX-Headers
    D3D12MemoryAllocator
)