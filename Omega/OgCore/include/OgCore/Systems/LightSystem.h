#pragma once
#define VK_NO_PROTOTYPES
#include <OgCore/Export.h>
#include <OgCore/Systems/System.h>
#include <OgCore/Components/LightSource.h>
#include <OgCore/Components/Transform.h>
#include <OgCore/Managers/SceneManager.h>
#include <OgRendering/Rendering/VulkanContext.h>
#include <memory>

namespace OgEngine
{

	class CORE_API LightSystem : public System
	{
	public:
		/**
		 * @brief This method is doing nothing right now but is a representation on how a system should be used.
		 * @note If you want the system to initialize things at the beginning of its creation you can do it here.
		 */
		void Init();

		/**
		* @brief Update the behaviour of an entity. Here we pass to the rendering the new model matrix so it can be updated in the final image.
		* @param p_dt The time elapsed between two frames
		* @param p_context The graphical context that can use specific methods between a raytraced and a rasterized pipeline.
		*/
		void Update(const float p_dt, const VulkanContext* p_context);
	};
}
