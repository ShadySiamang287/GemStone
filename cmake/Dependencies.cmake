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