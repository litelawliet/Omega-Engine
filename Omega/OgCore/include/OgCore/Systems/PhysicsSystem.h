#pragma once
#include <OgCore/Export.h>

#include <memory>

#include <OgCore/Systems/System.h>
#include <OgCore/Managers/SceneManager.h>
#include <OgCore/Components/RigidBody.h>
#include <OgCore/Components/Transform.h>
#include <OgPhysics/Physics.h>

namespace OgEngine
{

	class CORE_API PhysicsSystem : public System
	{
	public:
		/**
		 * @brief This method is doing nothing right now but is a representation on how a system should be used.
		 * @note If you want the system to initialize things at the beginning of its creation you can do it here.
		 */
		void Init();

		/**
		 * @brief Update the behaviour of an entity. Here we pass to physics engine the new physical properties of the entity so it can be updated in the physics engine.
		 * @param p_dt The time elapsed between two frames
		 * @param p_physicsEngine The physics engine that is used to simulate physics on entities.
		 */
		void Update(const float p_dt, PhysicsEngine& p_physicsEngine);
	};
}
