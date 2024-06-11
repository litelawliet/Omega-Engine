#include <cassert>
#include <OgPhysics/Physics.h>

using namespace physx;

UserErrorCallback OgEngine::PhysicsEngine::gDefaultErrorCallback;
PxDefaultAllocator OgEngine::PhysicsEngine::gDefaultAllocatorCallback;

OgEngine::PhysicsEngine::PhysicsEngine()
{
	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback,
		gDefaultErrorCallback);
	assert(m_foundation && "PxCreateFoundation failed.");

#ifdef _DEBUG
	const bool recordMemoryAllocations = true;
	
	m_pvd = PxCreatePvd(*m_foundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	m_pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation,
		PxTolerancesScale(), recordMemoryAllocations, m_pvd);
#else
	const bool recordMemoryAllocations = false;
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation,
		PxTolerancesScale(), recordMemoryAllocations, nullptr);
#endif

	assert(m_physics && "PxCreatePhysics failed.");

	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f * 5, 0.0f);
	sceneDesc.solverType = PxSolverType::eTGS;
	m_dispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = m_dispatcher;

	if (!sceneDesc.filterShader)
		sceneDesc.filterShader = &PxDefaultSimulationFilterShader;

	m_scene = m_physics->createScene(sceneDesc);
	assert(m_scene && "Physics Scene creation failed.");

	m_material = m_physics->createMaterial(0.5f, 0.5f, 0.1f);
}

OgEngine::PhysicsEngine::~PhysicsEngine()
{
	PX_RELEASE(m_material);
	PX_RELEASE(m_scene);
	PX_RELEASE(m_dispatcher);
	PX_RELEASE(m_physics);
	PX_RELEASE(m_foundation);
}

void OgEngine::PhysicsEngine::Update(const PxReal p_dt)
{
	m_accumulator += p_dt;
	if (m_accumulator < m_stepSize)
		return;

	m_accumulator -= m_stepSize;

	m_scene->simulate(m_stepSize);
	m_scene->fetchResults(true);
}

void OgEngine::PhysicsEngine::UpdateActor(PRigidBody* p_rb, bool isPlaying) const
{
	if (p_rb->rigidBody == nullptr)
		return;

	if (!isPlaying)
	{
		p_rb->rigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !p_rb->useGravity);
		p_rb->rigidBody->setMass(p_rb->mass);

		p_rb->rigidBody->detachShape(*p_rb->shape);
		p_rb->shape->release();

		if (p_rb->colliderType == RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_BOX)
			p_rb->shape = CreateBoxCollider(p_rb->shapeSizeX, p_rb->shapeSizeY, p_rb->shapeSizeZ);

		if (p_rb->colliderType == RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_SPHERE)
			p_rb->shape = CreateSphereCollider(p_rb->shapeSizeX);

		if (p_rb->colliderType == RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_PLANE)
			p_rb->shape = CreatePlaneCollider(p_rb->shapeSizeX, p_rb->shapeSizeZ);

		p_rb->rigidBody->attachShape(*p_rb->shape);
	}
}

void OgEngine::PhysicsEngine::SetWorldStep(const float p_newWorldStep)
{
	m_stepSize = p_newWorldStep;
}

inline physx::PxShape* OgEngine::PhysicsEngine::CreateSphereCollider(const float p_size) const
{
	return m_physics->createShape(physx::PxSphereGeometry(p_size), &m_material, true);
}

inline physx::PxShape* OgEngine::PhysicsEngine::CreateBoxCollider(const float p_sizeX, const float p_sizeY, const float p_sizeZ) const
{
	return m_physics->createShape(physx::PxBoxGeometry(p_sizeX, p_sizeY, p_sizeZ), &m_material, true);
}

inline physx::PxShape* OgEngine::PhysicsEngine::CreatePlaneCollider(const float p_sizeX, const float p_sizeZ) const
{
	return m_physics->createShape(physx::PxBoxGeometry(p_sizeX, 0.000001f, p_sizeZ), &m_material, true);
}

inline physx::PxMaterial* OgEngine::PhysicsEngine::GetDefaultMaterial() const
{
	return m_material;
}

inline physx::PxPhysics* OgEngine::PhysicsEngine::GetPhysics() const
{
	return m_physics;
}

float OgEngine::PhysicsEngine::WorldStep() const
{
	return m_stepSize;
}

void OgEngine::PhysicsEngine::AddRigidBodyToScene(PRigidBody* p_rigidBody, const physx::PxTransform&
	p_transform, const bool p_static) const
{
	p_rigidBody->rigidBody = m_physics->createRigidDynamic(p_transform);
	p_rigidBody->rigidBody->setSolverIterationCounts(16, 4);
	if (p_static)
		p_rigidBody->rigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

	p_rigidBody->rigidBody->attachShape(*p_rigidBody->shape);
	p_rigidBody->rigidBody->setMass(p_rigidBody->mass);
	m_scene->addActor(*p_rigidBody->rigidBody);
}

void OgEngine::PhysicsEngine::DeleteActor(PRigidBody* p_rb) const
{
	m_scene->removeActor(*p_rb->rigidBody);
	PX_RELEASE(p_rb->rigidBody);
}
