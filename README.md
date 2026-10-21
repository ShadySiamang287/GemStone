# Gemstone Engine
This is a DirectX 12 test engine im developing that uses slang shaders. It compiles as a dll which is neat i guess. 
## Dependencies
### Requirements

- C++ 23 Compatible Compiler
- CMake 3.28 or newer
- A DirectX 12 capabled windows system
- [GLM](https://github.com/g-truc/glm)
- [DirectX Headers](https://github.com/microsoft/DirectX-Headers)
- [DirectX12 Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator)
- DXC
### Installation
CMake automatically gathers and installs all dependencies assuming you have DXC on path, so just run build on either Debug or Build

## Documentation
[Read the GemStone API documentation](https://shadysiamang287.github.io/GemStone/)

### Offline generation of docs

Install [Doxygen](https://www.doxygen.nl/) and enable the optional CMake target:

```powershell
cmake -S . -B build -DGEMSTONE_BUILD_DOCUMENTATION=ON
cmake --build build --target documentation
```

The generated HTML documentation is written to `build/docs/html`.

