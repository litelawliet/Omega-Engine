#pragma once
template <typename ResourceType>
inline void OgEngine::ResourceManager::Add(std::string_view p_resourceName)
{
	std::cerr << "Warning: Unable to add the resource with type " << type_name<ResourceType>() << ".\n";
}

#pragma region Mesh
template <>
inline void OgEngine::ResourceManager::Add<OgEngine::Mesh>(const std::string_view p_resourceName)
{
	m_meshService.Add(p_resourceName);
}
#pragma endregion

#pragma region Texture
template <>
inline void OgEngine::ResourceManager::Add<OgEngine::Texture>(const std::string_view p_resourceName)
{
	m_textureService.Add(p_resourceName);
}
#pragma endregion

template <typename ResourceType>
inline ResourceType* OgEngine::ResourceManager::Get(std::string_view p_resourceName)
{
	std::cerr << "Warning: Unable to return the resource of type '" << type_name<ResourceType>() << "'.\n";
	return nullptr;
}

#pragma region Mesh
template <>
inline OgEngine::Mesh* OgEngine::ResourceManager::Get<OgEngine::Mesh>(
		const std::string_view p_resourceName)
{
	return m_meshService.Get(p_resourceName).get();
}
#pragma endregion

#pragma region Texture
template <>
inline OgEngine::Texture* OgEngine::ResourceManager::Get<OgEngine::Texture>(
		const std::string_view p_resourceName)
{
	return m_textureService.Get(p_resourceName).get();
}
#pragma endregion

template <typename ResourceType>
inline void OgEngine::ResourceManager::WaitForResource(std::string_view p_resourceName)
{
	std::cerr << "Warning: Unable to wait the resource of type '" << type_name<ResourceType>() << "'.\n";
}

#pragma region Mesh
template <>
inline void OgEngine::ResourceManager::WaitForResource<OgEngine::Mesh>(const std::string_view p_resourceName)
{
	m_meshService.WaitForResource(p_resourceName);
}
#pragma endregion

#pragma region Texture
template <>
inline void OgEngine::ResourceManager::WaitForResource<OgEngine::Texture>(const std::string_view p_resourceName)
{
	m_textureService.WaitForResource(p_resourceName);
}

inline std::vector<OgEngine::Texture*>& OgEngine::ResourceManager::GetAllTextures()
{
	return m_textureService.GetAllTextures();
}
#pragma endregion
