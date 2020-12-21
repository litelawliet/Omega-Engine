#include <OgCore/Components/RigidBody.h>

OgEngine::RigidBody::RigidBody(const RB_COLLIDER_TYPE p_type, const bool p_static)
	: m_shapeSizeX(m_rigidBody.shapeSizeX), m_shapeSizeY(m_rigidBody.shapeSizeY), m_shapeSizeZ(m_rigidBody.shapeSizeZ), m_mass(m_rigidBody.mass), m_rigidBodyType(p_type), m_useGravity(m_rigidBody.useGravity), m_isStatic(p_static)
{
	m_rigidBody.colliderType = p_type;

	
}

OgEngine::RigidBody::RigidBody(const RigidBody& p_other)
	: m_rigidBody(p_other.m_rigidBody), m_transform(p_other.m_transform), m_shapeSizeX(p_other.m_rigidBody.shapeSizeX), m_shapeSizeY(p_other.m_rigidBody.shapeSizeY), m_shapeSizeZ(p_other.m_rigidBody.shapeSizeZ), m_mass(p_other.m_rigidBody.mass), m_rigidBodyType(p_other.m_rigidBodyType), m_useGravity(p_other.m_rigidBody.useGravity), m_isStatic(p_other.m_isStatic)
{
}

OgEngine::RigidBody::RigidBody(RigidBody&& p_other) noexcept
	: m_rigidBody(std::move(p_other.m_rigidBody)), m_transform(p_other.m_transform), m_shapeSizeX(p_other.m_rigidBody.shapeSizeX), m_shapeSizeY(p_other.m_rigidBody.shapeSizeY), m_shapeSizeZ(p_other.m_rigidBody.shapeSizeZ), m_mass(p_other.m_rigidBody.mass), m_rigidBodyType(p_other.m_rigidBodyType), m_useGravity(p_other.m_rigidBody.useGravity), m_isStatic(p_other.m_isStatic)
{
}

void OgEngine::RigidBody::SetVelocity(const GPM::Vector3F& p_vec) const
{
	m_rigidBody.rigidBody->setLinearVelocity(physx::PxVec3(p_vec.x, p_vec.y, p_vec.z));
}

void OgEngine::RigidBody::CancelAllForces() const
{
	m_rigidBody.rigidBody->setLinearVelocity(physx::PxVec3(0));
	m_rigidBody.rigidBody->setAngularVelocity(physx::PxVec3(0));
	m_rigidBody.rigidBody->clearForce();
	m_rigidBody.rigidBody->clearTorque();
}

void OgEngine::RigidBody::SetLocalTransform(OgEngine::Transform& p_transform)
{
	m_transform = &p_transform;
}

void OgEngine::RigidBody::SetShapeSize(const float p_shapeSizeX, const float p_shapeSizeY, const float p_shapeSizeZ)
{

	m_shapeSizeX = p_shapeSizeX;
	m_shapeSizeY = p_shapeSizeY;
	m_shapeSizeZ = p_shapeSizeZ;
	m_rigidBody.shapeSizeX = p_shapeSizeX;
	m_rigidBody.shapeSizeY = p_shapeSizeY;
	m_rigidBody.shapeSizeZ = p_shapeSizeZ;
}

void OgEngine::RigidBody::SetMass(const float p_mass)
{
	m_mass = p_mass;
	m_rigidBody.mass = p_mass;
}

void OgEngine::RigidBody::EnableGravity(const bool p_gravityEnabled)
{
	m_useGravity = p_gravityEnabled;
	m_rigidBody.useGravity = p_gravityEnabled;
}

physx::PxTransform OgEngine::RigidBody::ConvertGPMtoPhysics(OgEngine::Transform* p_transform)
{
	if (p_transform)
	{
		const physx::PxQuat quaternion = physx::PxQuat(static_cast<float>(p_transform->rotation.x), static_cast<float>(p_transform->rotation.y),
			static_cast<float>(p_transform->rotation.z), static_cast<float>(p_transform->rotation.w)).getConjugate();
		const physx::PxVec3 position = physx::PxVec3(p_transform->position.x, p_transform->position.y, p_transform->position.z);

		return physx::PxTransform(position, quaternion);
	}

	return physx::PxTransform();
}

std::string OgEngine::RigidBody::Serialize(const int p_depth) const
{
	return std::string(DepthIndent(p_depth) + "<RigidBody>\n"
		// need to determine what to register here
		+ DepthIndent(p_depth + 1) + "<shapeSizeX>" + std::to_string(m_shapeSizeX) + "</shapeSizeX>\n"
		+ DepthIndent(p_depth + 1) + "<shapeSizeY>" + std::to_string(m_shapeSizeY) + "</shapeSizeY>\n"
		+ DepthIndent(p_depth + 1) + "<shapeSizeZ>" + std::to_string(m_shapeSizeZ) + "</shapeSizeZ>\n"
		+ DepthIndent(p_depth + 1) + "<mass>" + std::to_string(m_mass) + "</mass>\n"
		+ DepthIndent(p_depth + 1) + "<type>" + std::to_string(static_cast<uint8_t>(m_rigidBodyType)) + "</type>\n"
		+ DepthIndent(p_depth + 1) + "<gravity>" + std::to_string(m_useGravity) + "</gravity>\n"
		+ DepthIndent(p_depth + 1) + "<static>" + std::to_string(m_isStatic) + "</static>\n"
		+ DepthIndent(p_depth) + "</RigidBody>\n");
}

OgEngine::RigidBody& OgEngine::RigidBody::operator=(const RigidBody& p_other)
{
	if (&p_other == this)
		return *this;

	m_rigidBody = p_other.m_rigidBody;
	m_transform = p_other.m_transform;


	m_shapeSizeX = p_other.m_shapeSizeX;
	m_shapeSizeY = p_other.m_shapeSizeY;
	m_shapeSizeZ = p_other.m_shapeSizeZ;

	m_mass = p_other.m_mass;
	m_rigidBodyType = p_other.m_rigidBodyType;
	m_useGravity = p_other.m_useGravity;
	m_isStatic = p_other.m_isStatic;

	return *this;
}

OgEngine::RigidBody& OgEngine::RigidBody::operator=(RigidBody&& p_other) noexcept
{
	m_rigidBody = p_other.m_rigidBody;
	m_transform = p_other.m_transform;

	m_shapeSizeX = p_other.m_shapeSizeX;
	m_shapeSizeY = p_other.m_shapeSizeY;
	m_shapeSizeZ = p_other.m_shapeSizeZ;
	m_mass = p_other.m_mass;
	m_rigidBodyType = p_other.m_rigidBodyType;
	m_useGravity = p_other.m_useGravity;
	m_isStatic = p_other.m_isStatic;

	return *this;
}

void OgEngine::RigidBody::Initialize(PhysicsEngine& p_physics, float colliderSizeX, float colliderSizeY, float colliderSizeZ)
{
	m_rigidBody.shapeSizeX = colliderSizeX;
	m_rigidBody.shapeSizeY = colliderSizeY;
	m_rigidBody.shapeSizeZ = colliderSizeZ;

	if (m_rigidBodyType == RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_BOX)
		m_rigidBody.shape = p_physics.CreateBoxCollider(m_rigidBody.shapeSizeX, m_rigidBody.shapeSizeY, m_rigidBody.shapeSizeZ);
	else if (m_rigidBodyType == RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_SPHERE)
		m_rigidBody.shape = p_physics.CreateSphereCollider(m_rigidBody.shapeSizeX);
	else if (m_rigidBodyType == RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_PLANE)
		m_rigidBody.shape = p_physics.CreatePlaneCollider(m_rigidBody.shapeSizeX, m_rigidBody.shapeSizeZ);

	m_rigidBody.material = p_physics.GetPhysics()->createMaterial(0.5f, 0.5f, 0.8f);
}

std::string OgEngine::RigidBody::DepthIndent(const int p_depth)
{
	std::string depthCode;
	for (auto i = 0; i < p_depth; ++i)
	{
		depthCode += "\t";
	}

	return depthCode;
}

inline OgEngine::PRigidBody& OgEngine::RigidBody::GetRigidBody()
{
	return m_rigidBody;
}

inline bool OgEngine::RigidBody::UseGravity() const
{
	return m_useGravity;
}

bool OgEngine::RigidBody::IsStatic() const
{
	return m_isStatic;
}

inline float OgEngine::RigidBody::ShapeSizeX() const
{
	return m_shapeSizeX;
}

inline float OgEngine::RigidBody::ShapeSizeY() const
{
	return m_shapeSizeY;
}
inline float OgEngine::RigidBody::ShapeSizeZ() const
{
	return m_shapeSizeZ;
}

inline float OgEngine::RigidBody::Mass() const
{
	return m_mass;
}

OgEngine::Transform* OgEngine::RigidBody::Transform() const
{
	return m_transform;
}
