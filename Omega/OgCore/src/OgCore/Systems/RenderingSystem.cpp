#include<OgCore/Systems/RenderingSystem.h>
#include <OgCore/Components/Transform.h>
#include <OgCore/Managers/SceneManager.h>
#include <OgRendering/Rendering/VulkanContext.h>

void OgEngine::RenderingSystem::Init()
{
}

void OgEngine::RenderingSystem::Update(const float p_dt, const std::shared_ptr<VulkanContext>&  p_context)
{
	for (const auto& entity : m_entities)
	{
		auto& transform = SceneManager::GetComponent<Transform>(entity);

		auto& model = SceneManager::GetComponent<ModelRS>(entity);

		if (p_context->IsRaytracing())
		{
			p_context->GetRTPipeline()->UpdateObject(entity, transform.worldMatrix, model.Mesh(), rand() % 3);
		}
		else
		{
  			p_context->GetRSPipeline()->Update(p_dt, transform.worldMatrix, model.Mesh());
		}
	}
}
