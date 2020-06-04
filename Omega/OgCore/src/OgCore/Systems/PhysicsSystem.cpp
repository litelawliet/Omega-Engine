#include <OgCore/Systems/PhysicsSystem.h>

void OgEngine::PhysicsSystem::Init()
{
}

void OgEngine::PhysicsSystem::Update(const float p_dt, PhysicsEngine& p_physicsEngine)
{
	if (SceneManager::CurrentScene() == Scene::PLAY_SCENE)
	{
		p_physicsEngine.Update(p_dt);
		for (const auto& entity : m_entities)
		{
			auto& rigidBody = SceneManager::GetComponent<RigidBody>(entity);
			auto& tr = SceneManager::GetComponent<Transform>(entity);
			PRigidBody& rb = rigidBody.GetRigidBody();

			p_physicsEngine.UpdateActor(&rb, true);
			//tr.SetWorldMatrix(rigidBody.ConvertPhysicstoGPM(rb.rigidbody->getGlobalPose()));
			tr.SetPosition(GPM::Vector3(rb.rigidBody->getGlobalPose().p.x,
				rb.rigidBody->getGlobalPose().p.y,
				rb.rigidBody->getGlobalPose().p.z));

			tr.SetRotation(GPM::Quaternion(rb.rigidBody->getGlobalPose().q.x,
				rb.rigidBody->getGlobalPose().q.y,
				rb.rigidBody->getGlobalPose().q.z,
				rb.rigidBody->getGlobalPose().q.w));
		}
	}
	else if (SceneManager::CurrentScene() == Scene::EDITOR_SCENE)
	{
		for (const auto& entity : m_entities)
		{
			auto& rigidBody = SceneManager::GetComponent<RigidBody>(entity);
			auto& tr = SceneManager::GetComponent<Transform>(entity);

			PRigidBody& rb = rigidBody.GetRigidBody();
			p_physicsEngine.UpdateActor(&rb, false);

			if (rb.rigidBody != nullptr)
			{
				rb.rigidBody->setGlobalPose(rigidBody.ConvertGPMtoPhysics(&tr));
				rigidBody.CancelAllForces();
			}
		}
	}

}
