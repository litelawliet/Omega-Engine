#pragma once
#include <OgCore/Export.h>
#include <OgAudio/Audio/AudioEngine.h>
#include <OgRendering/Rendering/VulkanContext.h>
#include <OgCore/Systems/RenderingSystem.h>
#include <OgCore/Systems/PhysicsSystem.h>
#include <OgCore/Systems/LightSystem.h>
#include <OgCore/Systems/ScriptSystem.h>
#include <OgCore/SceneNode.h>
#include <OgPhysics/Physics.h>



namespace OgEngine
{
	class CORE_API Core final
	{
	public:
		/**
		 * @brief Constructor of the core engine, initialize internally the graphical context, the components and the systems.
		 * @param p_width The width of the window
		 * @param p_height The height of the window
		 * @param p_title The title of the window
		 */
		Core(const uint64_t p_width, const uint64_t p_height, const char* p_title);

		/**
		 * @brief Destructor, delete all the scene nodes in the scene graph.
		 */
		~Core();

		/**
		 * @brief Main method, this is the while loop where everything gets updated (systems, entities, inputs, rendering, etc.)
		 * @param p_dt Frame delta time
		 */
		void Run(float p_dt);

		/**
		 * @brief Tells the renderer to draw the final frame
		 */
		void Display() const;

#pragma region EDITOR_ECS_API
		/**
		 * @brief Add an entity to a scene node.
		 * @param p_parent The parent in which we will add a node. Default value is nullptr
		 * @note If no parameters are specified, the node will be added to the scene root.
		 */
		void AddEntity(SceneNode* p_parent = nullptr) const;

		/**
		 * @brief Remove an entity from the hierarchy.
		 * @param p_entity The entity to remove
		 */
		void DestroyEntityNode(SceneNode* p_entity) const;

		/**
		 * @brief Add a component to an entity
		 * @param p_entity The entity in which we add a new component
		 * @param p_component The new component to add
		 */
		template<typename T>
		inline void AddComponent(const Entity p_entity, T p_component) const;

		/**
		 * @brief Retrieve and give access to a component of an entity
		 * @param p_entity The entity to look for
		 * @return A reference to the component
		 */
		template<typename T>
		inline T& GetComponent(const Entity p_entity);

		/**
		 * @brief Remove a component from an entity
		 * @param p_entity The entity to look for
		 */
		template<typename T>
		inline void RemoveComponent(const Entity p_entity) const;

		/**
		 * @brief Tells if an Entity has a certain component.
		 * @param p_entity The entity to look for
		 */
		template<typename T>
		[[nodiscard]] inline bool HasComponent(const Entity p_entity) const;

#pragma endregion
		/**
		 * @brief Prepare the PLAY_SCENE to be played and emulating all systems.
		 */
		void PlayScene();

		/**
		 * @brief Prepare the EDITOR_SCENE to be played and cleaning the PLAY_SCENE that was previously playing.
		 */
		void EditorScene();

		/**
		 * @brief Save the EDITOR_SCENE into a scene file.
		 * @param p_sceneName Name of the scene
		 */
		void SaveScene(const std::string& p_sceneName);

		/**
		 * @brief Load a scene file into the editor.
		 * @param p_file The scene file to load
		 */
		void LoadScene(const std::string& p_file);

		/**
		 * @brief Remove all remaining children recursively from the renderer.
		 * @param p_parent The parent node
		 */
		void RemoveRenderedObjects(SceneNode* p_parent) const;


		/**
		 * @brief Remove all remaining children recursively from the renderer.
		 * @param p_file The file to write in
		 * @param p_parent The parent to start recording from
		 * @param p_depth The depth of the current group of data
		 */
		void SerializeChildren(std::ostream& p_file, SceneNode* p_parent, int& p_depth);

		/**
		 * @brief Add a handle to a texture in the current renderer.
		 * @param p_texture The texture to add
		 * @param p_textureType The type of the texture (albedo map or normal map)
		 */
		void AddTexture(const std::string& p_texture, const TEXTURE_TYPE p_textureType) const;

		/**
		 * @brief Add a rigidBody to the physics engine
		 * @param p_entity The entity holding the rigidBody
		 */
		void AddRigidBodyToPhysics(const Entity p_entity);
		
		/**
		 * @brief Remove all remaining children recursively from the renderer.
		 * @param p_depth The depth to indent
		 */
		[[nodiscard]] static std::string DepthIndent(const int p_depth);

		/**
		 * @brief Register all Components and Systems in a specific Scene, needed in all scene as ECS is templated and need to allocate everything at compile time.
		 * @param p_scene The concern scene in which we register the components
		 * @note The left parameter is the PLAY_SCENE scene graph to fill in. The right parameter correspond to the EDITOR_SCENE scene graph to be reproduced.
		 */
		void RegisterComponentsAndSystems(const Scene& p_scene);

		/**
		 * @brief Create all the children of each parent until the end
		 * @param p_parent The parent in which to add all the children (adding in EDITOR_SCENE).
		 * @param p_parentToReproduce Parent hierarchy to reproduce (from EDITOR_SCENE)
		 * @note The left parameter is the PLAY_SCENE scene graph to fill in. The right parameter correspond to the EDITOR_SCENE scene graph to be reproduced.
		 */
		void CreateChildrenOf(SceneNode* p_parent, SceneNode* p_parentToReproduce) const;

		Core(const Core& p_other) = delete;
		Core(Core&& p_other) = delete;
		Core& operator=(const Core& p_other) = delete;
		Core& operator=(Core&& p_other) = delete;
	public:
		AudioEngine m_audioEngine;
		PhysicsEngine m_physicsEngine;

		VulkanContext* m_vulkanContext;

		std::array<std::shared_ptr<RenderingSystem>, 2> m_renderSystem;
		std::array<std::shared_ptr<PhysicsSystem>, 2> m_physicsSystem;
		std::array<std::shared_ptr<LightSystem>, 2> m_lightSystem;
		std::array<std::shared_ptr<ScriptSystem>, 2> m_scriptSystem;

		std::array<SceneNode*, 2> roots = { nullptr, nullptr };

		SceneNode* inspectorNode = nullptr;
	};
}

#include <OgCore/Core.inl>