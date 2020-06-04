#include <OgCore/Components/CustomScript.h>

OgEngine::CustomScript::CustomScript()
	: x(0.0f)
{
}

void OgEngine::CustomScript::Start()
{
}

void OgEngine::CustomScript::Update(const float p_dt)
{
	x += p_dt;
	transform->Translate(GPM::Vector3F(0.0f, 0.0f, p_dt));
}
