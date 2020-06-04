#include <OgCore/Components/AScript.h>

OgEngine::AScript::AScript()
	: transform(nullptr)
{
}

OgEngine::AScript::~AScript()
{
}

void OgEngine::AScript::Start()
{
	if (runningScript)
	{
		runningScript->Start();
	}
}

void OgEngine::AScript::Update(const float p_dt)
{
	if (runningScript)
	{
		runningScript->Update(p_dt);
	}
}

void OgEngine::AScript::SetLocalTransform(Transform& p_transform)
{
	transform = &p_transform;
}

void OgEngine::AScript::SetRunningScript(std::shared_ptr<AScript> p_script)
{
	runningScript = std::move(p_script);
	runningScript->SetLocalTransform(*transform);
}