#include <OgRendering/Managers/Loaders/ShaderLoader.h>

#include <iostream>
#include <fstream>
#include <cassert>

VkShaderModule ShaderLoader::LoadShader(const char* p_fileName, VkDevice p_device)
{
	std::ifstream is(p_fileName, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open())
	{
		const size_t size = is.tellg();
		char* shaderCode = new char[size];

		assert(size > 0);

		is.seekg(0);
		is.read(shaderCode, size);
		is.close();

		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode);

		vkCreateShaderModule(p_device, &moduleCreateInfo, nullptr, &shaderModule);

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << p_fileName << "\"" << std::endl;
		return nullptr;
	}
}
