#pragma once
#include <iostream>
#include <OgPhysics/Export.h>
#include <PxPhysicsAPI.h>
#include <extensions/PxDefaultAllocator.h>
#include <extensions/PxDefaultSimulationFilterShader.h>

template <typename T>
constexpr void PX_RELEASE(T x)
{
	if (x)
	{
		x->release();
		x = nullptr;
	}
}

constexpr const char* PVD_HOST = "127.0.0.1";

class UserErrorCallback : public physx::PxErrorCallback
{
public:
	void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file,
		int line) override
	{
		std::cerr << "Error physics: " << message << '\n';
	}
};

namespace OgEngine
{
	enum RB_COLLIDER_TYPE
	{
		RB_COLLIDER_TYPE_SPHERE,
		RB_COLLIDER_TYPE_BOX,
		RB_COLLIDER_TYPE_PLANE
	};

	struct PRigidBody final
	{
		physx::PxRigidDynamic* rigidBody = nullptr;
		physx::PxShape* shape = nullptr;
		physx::PxMaterial* material = nullptr;
		float shapeSizeX{ 1.0f };
		float shapeSizeY{ 1.0f };
		float shapeSizeZ{ 1.0f };
		float mass{ 1.0f };
		bool useGravity{ true };
		bool isStatic{ false };
		RB_COLLIDER_TYPE colliderType;

		PRigidBody(const RB_COLLIDER_TYPE p_colliderType = RB_COLLIDER_TYPE_BOX)
			: shapeSizeX(1.0f), shapeSizeY(1.0f), shapeSizeZ(1.0f), mass(1.0f), useGravity(true), isStatic(false), colliderType(p_colliderType)
		{
			
		}
		PRigidBody(const PRigidBody& p_other)
			: rigidBody(p_other.rigidBody), shape(p_other.shape), material(p_other.material), shapeSizeX(p_other.shapeSizeX), shapeSizeY(p_other.shapeSizeY), shapeSizeZ(p_other.shapeSizeZ), mass(p_other.mass), useGravity(p_other.useGravity), isStatic(p_other.isStatic), colliderType(p_other.colliderType)
		{
		}

		PRigidBody(PRigidBody&& p_other) noexcept
			: rigidBody(p_other.rigidBody), shape(p_other.shape), material(p_other.material), shapeSizeX(p_other.shapeSizeX), shapeSizeY(p_other.shapeSizeY), shapeSizeZ(p_other.shapeSizeZ), mass(p_other.mass), useGravity(p_other.useGravity), isStatic(p_other.isStatic), colliderType(p_other.colliderType)
		{
		}



		inline PRigidBody& operator=(const PRigidBody& p_other)
		{
			if (&p_other == this)
				return *this;

			rigidBody = p_other.rigidBody;
			shape = p_other.shape;
			material = p_other.material;

			shapeSizeX = p_other.shapeSizeX;
			shapeSizeY = p_other.shapeSizeY;
			shapeSizeZ = p_other.shapeSizeZ;

			mass = p_other.mass;
			useGravity = p_other.useGravity;
			isStatic = p_other.isStatic;
			colliderType = p_other.colliderType;

			return *this;
		}

		inline PRigidBody& operator=(PRigidBody&& p_other) noexcept
		{
			rigidBody = p_other.rigidBody;
			shape = p_other.shape;
			material = p_other.material;

			shapeSizeX = p_other.shapeSizeX;
			shapeSizeY = p_other.shapeSizeY;
			shapeSizeZ = p_other.shapeSizeZ;
			mass = p_other.mass;
			useGravity = p_other.useGravity;
			isStatic = p_other.isStatic;
			colliderType = p_other.colliderType;

			return *this;
		}
	};


	class PHYSICS_API PhysicsEngine
	{
	public:
		PhysicsEngine();
		~PhysicsEngine();

		static UserErrorCallback gDefaultErrorCallback;
		static physx::PxDefaultAllocator gDefaultAllocatorCallback;

		void Update(const physx::PxReal p_dt);

		void DeleteActor(PRigidBody* p_rb) const;
		void UpdateActor(PRigidBody* p_rb, bool isPlaying) const;

		inline void SetWorldStep(const float p_newWorldStep);

		[[nodiscard]] inline physx::PxShape* CreateSphereCollider(const float p_size) const;
		[[nodiscard]] inline physx::PxShape* CreateBoxCollider(const float p_sizeX, const float p_sizeY, const float p_sizeZ) const;
		[[nodiscard]] inline physx::PxShape* CreatePlaneCollider(const float p_sizeX, const float p_sizeZ) const;

		[[nodiscard]] inline physx::PxMaterial* GetDefaultMaterial() const;
		[[nodiscard]] inline physx::PxPhysics* GetPhysics() const;
		[[nodiscard]] inline float WorldStep() const;

		void AddRigidBodyToScene(PRigidBody* p_rigidBody, const physx::PxTransform& p_transform, const bool p_static) const;

	private:
		physx::PxFoundation* m_foundation = nullptr;
		physx::PxPvd* m_pvd = nullptr;
		physx::PxPhysics* m_physics = nullptr;
		physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;

		physx::PxScene* m_scene = nullptr;
		physx::PxMaterial* m_material = nullptr;

		physx::PxReal m_stepSize = 1.0f / 60.0f;
		physx::PxReal m_accumulator = 0.0f;
	};
}