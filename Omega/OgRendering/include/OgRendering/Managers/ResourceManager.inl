#pragma once

#pragma region UnsupportedTypes
template <typename ResourceType>
void OgEngine::ResourceManager::Add(std::string_view p_resourceName)
{
	std::cerr << "Warning: Unable to add the resource with type " << type_name<ResourceType>() << ".\n";
}

template <typename ResourceType>
ResourceType* OgEngine::ResourceManager::Get(std::string_view p_resourceName)
{
	std::cerr << "Warning: Unable to return the resource of type '" << type_name<ResourceType>() << "'.\n";
	return nullptr;
}

template <typename ResourceType>
void OgEngine::ResourceManager::WaitForResource(std::string_view p_resourceName)
{
	std::cerr << "Warning: Unable to wait the resource of type '" << type_name<ResourceType>() << "'.\n";
}
#pragma endregion

#pragma region Mesh
template <>
void OgEngine::ResourceManager::Add<OgEngine::Mesh>(const std::string_view p_resourceName)
{
	m_meshService.Add(p_resourceName);
}

template <>
OgEngine::Mesh* OgEngine::ResourceManager::Get<OgEngine::Mesh>(
	const std::string_view p_resourceName)
{
	return &(*m_meshService.Get(p_resourceName).get());
}

template <>
void OgEngine::ResourceManager::WaitForResource<OgEngine::Mesh>(const std::string_view p_resourceName)
{
	m_meshService.WaitForResource(p_resourceName);
}
#pragma endregion

#pragma region Texture
template <>
void OgEngine::ResourceManager::Add<OgEngine::Texture>(const std::string_view p_resourceName)
{
	m_textureService.Add(p_resourceName);
}

template <>
OgEngine::Texture* OgEngine::ResourceManager::Get<OgEngine::Texture>(
	const std::string_view p_resourceName)
{
	return &(*m_textureService.Get(p_resourceName).get());
}

template <>
void OgEngine::ResourceManager::WaitForResource<OgEngine::Texture>(const std::string_view p_resourceName)
{
	m_textureService.WaitForResource(p_resourceName);
}

std::vector<OgEngine::Texture*>& OgEngine::ResourceManager::GetAllTextures()
{
	return m_textureService.GetAllTextures();
}

inline void OgEngine::ResourceManager::WaitForAll()
{
	m_textureService.WaitForAll();
	m_meshService.WaitForAll();
}
#pragma endregion
