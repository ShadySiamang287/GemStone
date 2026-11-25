#include "Graphics/GraphicsPipeline.h"

using namespace GEM;

ShaderBytecode::ShaderBytecode(std::filesystem::path path) {
    std::ifstream file(path,std::ios::binary | std::ios::ate);

    if (!file)
    {
        throw std::runtime_error("Failed to open shader: " + path.string());
    }

    const std::streampos end = file.tellg();

    if (end < 0)
    {
        throw std::runtime_error("Failed to determine shader size: " + path.string());
    }

    const auto size = static_cast<std::size_t>(end);

    data.resize(size);

    file.seekg(0, std::ios::beg);

    if (size > 0 &&
        !file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size)))
    {
        throw std::runtime_error("Failed to read shader: " +path.string());
    }
}