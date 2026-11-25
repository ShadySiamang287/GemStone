include(FetchContent)

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

fetchcontent_declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)

fetchcontent_makeavailable(DirectXHeaders D3D12MemoryAllocator glm)

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

add_library(imgui SHARED)

set_target_properties(imgui PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

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

FetchContent_Declare(
    agility
    URL           https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.619.6
    DOWNLOAD_NAME agility.zip
)
FetchContent_MakeAvailable(agility)

file(GLOB_RECURSE _agility_all RELATIVE "${agility_SOURCE_DIR}" "${agility_SOURCE_DIR}/*")
list(FILTER _agility_all INCLUDE REGEX "(^|/)x64(/|$)")
set(AGILITY_CORE   "")
set(AGILITY_LAYERS "")
foreach(f ${_agility_all})
    if(f MATCHES "(^|/)D3D12Core\\.dll$")
        set(AGILITY_CORE   "${agility_SOURCE_DIR}/${f}")
    elseif(f MATCHES "(^|/)d3d12SDKLayers\\.dll$")
        set(AGILITY_LAYERS "${agility_SOURCE_DIR}/${f}")
    endif()
endforeach()
if(NOT AGILITY_CORE)
    message(FATAL_ERROR "Agility SDK: D3D12Core.dll (x64) not found in ${agility_SOURCE_DIR}")
endif()