#pragma once
#include <OgCore/Export.h>
#include <memory>
#include <array>
#include <OgCore/Entities/Types.h>
#include <OgCore/Managers/ComponentManager.h>
#include <OgCore/Managers/EntityManager.h>
#include <OgCore/Managers/SystemManager.h>

namespace OgEngine
{
	enum class CORE_API Scene : std::uint8_t
	{
		EDITOR_SCENE = 0u,
		PLAY_SCENE = 1u,
		COUNT = 2u
	};

	class CORE_API SceneManager
	{
	public:
#pragma region SceneManagerMethods
		/**
		 * @brief Change the scene currently used by a new one
		 * @param p_newScene The new scene to use [EDITOR_SCENE=0; PLAY_SCENE=1]
		 * @note There are only 2 scenes in this SceneManager. One for the playing mode, and one for the editor mode.
		 */
		static void ChangeScene(const OgEngine::Scene& p_newScene);

		/**
		* @brief Create an entity
		* @note The method will fail if you try to create more entity than the maximum supported AND alive entities (5000)
		*/
		[[nodiscard]] static OgEngine::Scene CurrentScene();
#pragma endregion 
		
#pragma region EntitiesMethods
		/**
		 * @brief Create an entity
		 * @note The method will fail if you try to create more entity than the maximum supported AND alive entities (5000)
		 */
		[[nodiscard]] static Entity CreateEntity();

		/**
		 * @brief Destroy an entity and all the components associated. It also remove the entity from the systems who uses the entity
		 * @param p_entity The entity to destroy
		 * @note The method will fail if the entity doesn't exist
		 */
		static void                 DestroyEntity(const Entity p_entity);
#pragma endregion
#pragma region ComponentsMethods
		/**
		 * @brief Register a component into a component array
		 * @note It is very important to register all the component you will use in the program before using it. Otherwise the component will not exist and will throw an exception.
		 */
		template <typename T>
		static void RegisterComponent();

		/**
		 * @brief Add a component to an entity
		 * @param p_entity The entity to which we add a component
		 * @param p_component The component to add
		 * @note The method will fail if the entity doesn't exist or if the component already exists for this entity
		 */
		template <typename T>
		static void AddComponent(const Entity p_entity, T p_component);

		/**
		 * @brief Remove a component of an entity
		 * @param p_entity The entity to which we remove a component
		 * @note The method will fail if the entity doesn't exist or if the component doesn't exist for this entity
		 */
		template <typename T>
		static void RemoveComponent(const Entity p_entity);

		/**
		 * @brief Get a component of an entity
		 * @param p_entity The entity to which we get a component
		 * @return The component of the entity
		 * @note The method will fail if the entity doesn't exist or if the component doesn't exist for this entity
		 */
		template <typename T>
		static T& GetComponent(const Entity p_entity);

		/**
		 * @brief Return the component's type of a component array
		 * @return The component's of a component array
		 */
		template <typename T>
		static ComponentType GetComponentType();

		/**
		 * @brief Return true or false if an Entity has a certain component
		 */
		template <typename T>
		static bool HasComponent(const Entity p_entity);

		/**
		 * @brief Return the signature of an entity
		 */
		static Signature GetSignature(const Entity p_entity);
#pragma endregion
#pragma region Systems methods
		/**
		 * @brief Register a system, gives the records and handle of entities automatically.
		 * @return Return a shared_ptr of a valid system class.
		 * @note You need to register a system before using it even if it is a valid class. Note that a valid system is a system who inherit from the System class.
		 */
		template <typename T>
		static std::shared_ptr<T> RegisterSystem();

		/**
		 * @brief Set a signature to a system
		 * @param p_signature The new signature of a system
		 * @note A signature is a bitfield where each bit correspond to a specific component defining what component a system needs in order to work properly. The method will fail if the system is not registered yet.
		 */
		template <typename T>
		static void SetSystemSignature(const Signature p_signature);
#pragma endregion
	private:
		static std::array<std::unique_ptr<ComponentManager>, 2> m_componentManager;
		static std::array<std::unique_ptr<EntityManager>, 2>    m_entityManager;
		static std::array<std::unique_ptr<SystemManager>, 2>    m_systemManager;
		static OgEngine::Scene m_currentScene;
	};
}

#include <OgCore/Managers/SceneManager.inl>
