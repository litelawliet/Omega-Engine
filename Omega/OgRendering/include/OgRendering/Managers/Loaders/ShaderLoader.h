#pragma once
#include <OgRendering/Utils/volk.h>

class ShaderLoader final
{

public:
    ShaderLoader() = default;
    ~ShaderLoader() = default;

    static VkShaderModule LoadShader(const char* p_fileName, VkDevice p_device);
};