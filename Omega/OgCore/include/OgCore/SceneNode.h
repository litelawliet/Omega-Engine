#pragma once
#include <list>
#include <OgCore/Export.h>

#include <glm/glm.hpp>

#include <OgCore/Entities/Types.h>
#include <vector>

namespace OgEngine
{
	class CORE_API SceneNode
	{
	public:
		/**
		 * @brief Default constructor, need an entity attached to it
		 * @param p_entity The entity to create
		 */
		SceneNode(Entity p_entity);

		/**
		 * @brief Destroy the scene node and the entity attached to it as well as all the children of that scene node.
		 */
		~SceneNode();

		/**
		 * @brief Return the world transform of the entity of this scene node
		 * @return The world matrix
		 * @note No discard qualifier.
		 */
		[[nodiscard]] glm::mat4 GetWorldTransform() const;

		/**
		 * @brief Return the entity of this scene node
		 * @return The entity
		 * @note No discard qualifier.
		 */
		[[nodiscard]] Entity GetEntity() const;
		
		/**
		 * @brief Add a child to this scene node
		 * @param p_childNode The child node to add
		 */
		void AddChild(SceneNode* p_childNode);

		/**
		 * @brief Return a child using index from this scene node
		 * @param p_childIndex The child node index
		 * @return The child of this scene node at a certain index
		 */
		SceneNode* GetChild(const uint64_t p_childIndex);

		/**
		 * @brief Return the last added child
		 * @return The last child
		 * @note If no child exist yet, the function will return nullptr
		 */
		[[nodiscard]] SceneNode* LastChild() const;

		/**
		 * @brief Return a reference on the set of children of this scene node
		 * @return The set of children
		 */
		std::vector<SceneNode*>& GetChildren();

		/**
		 * @brief Return the number of children of this scene node
		 * @return The number of children
		 * @note No discard qualifier.
		 */
		[[nodiscard]] uint64_t ChildCount() const;

		/**
		 * @brief Update the world matrices of all the scene node and their children
		 * @param p_dt The time elapsed between two frames
		 */
		void Update(float p_dt);

		/**
		 * @brief Return the const iterator on the beginning of the children set (read-only).
		 * @return The const iterator over the first child of the set
		 */
		std::vector<SceneNode*>::const_iterator GetChildIteratorStart();

		/**
		 * @brief Return the const iterator on the ending of the children set (read-only).
		 * @return The const iterator over the last child of the set
		 */
		std::vector<SceneNode*>::const_iterator GetChildIteratorEnd();

		/**
		 * @brief Get the parent SceneNode
		 * @return The parent
		 * @note Return nullptr if there is no parent (root)
		 */
		[[nodiscard]] SceneNode* GetParent() const;

		/**
		 * @brief Tell if this node has a parent node
		 * @return True or false
		 */
		[[nodiscard]] bool HasParent() const;

		/**
		 * @brief Remove a child from a node Scene  using index
		 * @param p_index Index of the child to remove
		 */
		void RemoveChild(const uint64_t p_index);

		/**
		 * @brief Remove a child from a node Scene using SceneNode address.
		 * @param p_childNode Child node to remove
		 */
		void RemoveChild(SceneNode* p_childNode);

		/**
		 * @brief Remove all the children
		 */
		void RemoveChildren();

	protected:
		SceneNode* m_parentNode;
		Entity m_entity;
		std::vector<SceneNode*> m_children;
	};
}
