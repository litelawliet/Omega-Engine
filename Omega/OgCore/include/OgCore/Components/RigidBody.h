#pragma once
#include <OgCore/Export.h>
#include <OgPhysics/Physics.h>
#include <OgCore/Components/Transform.h>
#include <GPM/GPM.h>

namespace OgEngine
{

	class CORE_API RigidBody final
	{
	public:
		RigidBody() = default;
		RigidBody(const RB_COLLIDER_TYPE p_type, const bool p_static);
		RigidBody(const RigidBody& p_other);
		RigidBody(RigidBody&& p_other) noexcept;
		~RigidBody() = default;
		
		void SetVelocity(const GPM::Vector3F& p_vec) const;
		void CancelAllForces() const;
		void SetLocalTransform(OgEngine::Transform& p_transform);

		void SetShapeSize(const float p_shapeSizeX, const float p_shapeSizeY, const float p_shapeSizeZ);
		void SetMass(const float p_mass);
		void EnableGravity(const bool p_gravityEnabled);

		[[nodiscard]] inline PRigidBody& GetRigidBody();
		[[nodiscard]] inline bool UseGravity() const;
		[[nodiscard]] inline bool IsStatic() const;
		[[nodiscard]] inline float ShapeSizeX() const;
		[[nodiscard]] inline float ShapeSizeY() const;
		[[nodiscard]] inline float ShapeSizeZ() const;
		[[nodiscard]] inline float Mass() const;
		[[nodiscard]] inline OgEngine::Transform* Transform() const;

		static physx::PxTransform ConvertGPMtoPhysics(OgEngine::Transform* p_transform);
		GPM::Matrix4F ConvertPhysicstoGPM(const physx::PxTransform& p_transform);
		[[nodiscard]] std::string Serialize(const int p_depth) const;

		inline RigidBody& operator=(const RigidBody& p_other);
		inline RigidBody& operator=(RigidBody&& p_other) noexcept;
		void Initialize(PhysicsEngine& p_physics, float colliderSizeX, float colliderSizeY, float colliderSizeZ);

	private:
		PRigidBody m_rigidBody;
		OgEngine::Transform* m_transform;

		float m_shapeSizeX = 1.0f;
		float m_shapeSizeY = 1.0f;
		float m_shapeSizeZ = 1.0f;

		float m_mass = 1.0f;
		RB_COLLIDER_TYPE m_rigidBodyType;
		bool m_useGravity = true;
		bool m_isStatic = false;

		[[nodiscard]] static std::string DepthIndent(const int p_depth);
	};
}
