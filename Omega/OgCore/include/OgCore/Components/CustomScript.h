#pragma once

#include <OgCore/Export.h>
#include <OgCore/Components/AScript.h>

namespace OgEngine
{
	class CORE_API CustomScript : public AScript
	{
	public:
		CustomScript();

		void Start() override;
		void Update(const float p_dt) override;

	private:
		float x;
	};
}
