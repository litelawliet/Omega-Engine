#include<OgCore/Systems/RenderingSystem.h>



void OgEngine::RenderingSystem::Init()
{
}

void OgEngine::RenderingSystem::Update(const float p_dt, const VulkanContext* p_context)
{
	for (const auto& entity : m_entities)
	{
		auto& transform = SceneManager::GetComponent<Transform>(entity);
		auto& model = SceneManager::GetComponent<ModelRS>(entity);

		if (p_context->IsRaytracing())
		{
			p_context->GetRTPipeline()->UpdateObject(
				entity,
				transform.worldMatrix,
				model.GetMesh(),
				model.Material().texName.c_str(),
				model.Material().normName.c_str(),
				model.Material().color,
				model.Material().roughness,
				model.Material().ior,
				model.Material().specular,
				model.Material().emissive,
				model.Material().type
			);
		}
		else
		{
			p_context->GetRSPipeline()->Update(
				p_dt,
				entity, 
				transform.worldMatrix,
				model.GetMesh(),
				model.Material().texName,
				model.Material().normName,
				model.Material().color
			);
		}
	}
}
