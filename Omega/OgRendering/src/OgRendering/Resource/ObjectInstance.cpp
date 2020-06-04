#include <OgRendering/Resource/ObjectInstance.h>

OgEngine::ObjectInstance::ObjectInstance(OgEngine::Mesh* p_mesh)
	: instanceID(0u)
{
	model.SetMesh(p_mesh);
}

bool OgEngine::ObjectInstance::operator==(const ObjectInstance& p_other) const
{
	return instanceID == p_other.instanceID;
}

bool OgEngine::ObjectInstance::operator!=(const ObjectInstance& p_other) const
{
	return instanceID != p_other.instanceID;
}
