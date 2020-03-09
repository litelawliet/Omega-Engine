#include <OgCore/Components/Material.h>

OgEngine::Material::Material()
	: color(Vector3F::one), rough(0.0), metal(false)
{
}

void OgEngine::Material::SetLocalTransform(Transform& p_transform)
{
	m_lightTransform = &(p_transform);
}
