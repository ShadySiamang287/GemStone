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