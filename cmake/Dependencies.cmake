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