#include <OgCore/Systems/ScriptSystem.h>

void OgEngine::ScriptSystem::Init()
{
}

void OgEngine::ScriptSystem::Update(const float p_dt, const VulkanContext* p_context)
{
	if (SceneManager::CurrentScene() == Scene::PLAY_SCENE)
	{
		for (const auto& entity : m_entities)
		{
			auto& script = SceneManager::GetComponent<AScript>(entity);
			auto& transform = SceneManager::GetComponent<Transform>(entity);

			script.Update(p_dt);
		}
	}
}
