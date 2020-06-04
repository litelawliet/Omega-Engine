#include <OgRendering/Managers/ResourceManager.h>

OgEngine::Services::MeshService    OgEngine::ResourceManager::m_meshService{};
OgEngine::Services::TextureService OgEngine::ResourceManager::m_textureService{};
bool                               OgEngine::ResourceManager::m_raytracingEnable = false;

OgEngine::ResourceManager& OgEngine::ResourceManager::Instance()
{
	static ResourceManager m_instance;

	return m_instance;
}
