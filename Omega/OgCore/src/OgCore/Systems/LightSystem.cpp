#include <OgCore/Systems/LightSystem.h>

void OgEngine::LightSystem::Init()
{
}

void OgEngine::LightSystem::Update(const float p_dt, const VulkanContext* p_context)
{
    for (const auto& entity : m_entities)
    {
		auto& light = SceneManager::GetComponent<LightSource>(entity);
		auto& transform = SceneManager::GetComponent<Transform>(entity);

		if (p_context->IsRaytracing())
		{
			GPM::Vector4F lightPos = GPM::Vector4F({ transform.position.x, transform.position.y, transform.position.z, 1 });
			p_context->GetRTPipeline()->UpdateLight(entity, lightPos, light.color, light.direction, light.lightType);
		}
    }
}