# Gemstone Engine
This is a DirectX 12 test engine im developing that uses slang shaders. It compiles as a dll which is neat i guess. 
## Dependencies
### Requirements

- C++ 23 Compatible Compiler
- CMake 3.28 or newer
- A DirectX 12 capabled windows system
- [DirectXMath](https://github.com/microsoft/DirectXMath)
- [DirectX Headers](https://github.com/microsoft/DirectX-Headers)
- [DirectX12 Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator)
- [Slang](https://github.com/shader-slang/slang)
### Installation
CMake automatically gathers and installs all dependencies **except slang**, so just run build on either Debug or Build after installing slangc

## Documentation

Install [Doxygen](https://www.doxygen.nl/) and enable the optional CMake target:

```powershell
cmake -S . -B build -DGEMSTONE_BUILD_DOCUMENTATION=ON
cmake --build build --target documentation
```

The generated HTML documentation is written to `build/docs/html`. The GitHub Actions workflow generates the same documentation and uploads it as a workflow artifact.
