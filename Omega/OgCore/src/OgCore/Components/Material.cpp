#include <OgCore/Components/Material.h>

OgEngine::Material::Material()
	: color(Vector3F::one), roughness(0.0), materialType(2)
{
}

void OgEngine::Material::SetLocalTransform(Transform& p_transform)
{
	m_lightTransform = &(p_transform);
}
