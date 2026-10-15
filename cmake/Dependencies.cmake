include(FetchContent)

fetchcontent_declare(
    DirectXMath
    GIT_REPOSITORY https://github.com/microsoft/DirectXMath.git
    GIT_TAG main
)

fetchcontent_declare(
  DirectXHeaders 
  GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
  GIT_TAG main
)

fetchcontent_declare(
  D3D12MemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
  GIT_TAG master
)

fetchcontent_makeavailable(DirectXMath DirectXHeaders D3D12MemoryAllocator)

FetchContent_Declare(
    WinPixEventRuntime
    URL https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.240308001
    DOWNLOAD_NAME WinPixEventRuntime.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(WinPixEventRuntime)

set(PIX_DIR "${winpixeventruntime_SOURCE_DIR}")

add_library(WinPixEventRuntime SHARED IMPORTED)
set_target_properties(WinPixEventRuntime PROPERTIES
    IMPORTED_IMPLIB   "${PIX_DIR}/bin/x64/WinPixEventRuntime.lib"
    IMPORTED_LOCATION "${PIX_DIR}/bin/x64/WinPixEventRuntime.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${PIX_DIR}/Include/WinPixEventRuntime"
    INTERFACE_COMPILE_DEFINITIONS "USE_PIX"
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        docking
  )

FetchContent_MakeAvailable(imgui)

add_library(imgui)

target_include_directories(imgui
    PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_sources(
  imgui
  PRIVATE
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)

target_link_libraries(imgui PUBLIC d3d12 dxgi d3dcompiler)
