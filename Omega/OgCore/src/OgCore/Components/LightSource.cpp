#include <OgCore/Components/LightSource.h>
#include <OgCore/Components/Transform.h>

OgEngine::LightSource::LightSource()
= default;

void OgEngine::LightSource::SetLocalTransform(Transform& p_transform)
{
	m_lightTransform = &(p_transform);
	position = m_lightTransform->position;
}
