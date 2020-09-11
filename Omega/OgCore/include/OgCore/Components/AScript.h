#pragma once

#include <OgCore/Export.h>

#include <OgCore/Components/Transform.h>
#include <memory>

namespace OgEngine
{
	class CORE_API AScript
	{
	public:
		AScript();

		virtual ~AScript();

		// You can override 
		virtual void Start();

		virtual void Update(const float p_dt);

		void SetLocalTransform(Transform& p_transform);
		void SetRunningScript(std::shared_ptr<AScript> p_script);
		
	protected:
		Transform* transform;
		std::shared_ptr<AScript> runningScript;
		std::string scriptName;
	};
}
