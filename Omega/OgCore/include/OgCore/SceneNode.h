#pragma once


#include <GPM/GPM.h>

#include <OgCore/Entities/Entity.h>
#include <vector>

namespace OgEngine
{
	class SceneNode
	{
	public:
		SceneNode(Entity p_entity);
		~SceneNode();

		[[nodiscard]] Matrix4F GetWorldTransform() const;
		[[nodiscard]] Entity GetEntity() const;
		
		void AddChild(SceneNode* p_childNode);
		SceneNode* GetChild(const uint64_t p_childIndex);

		std::vector<SceneNode*>& GetChildren();

		[[nodiscard]] uint64_t ChildCount() const;

		void Update(float p_dt);

		std::vector<SceneNode*>::const_iterator GetChildIteratorStart();
		std::vector<SceneNode*>::const_iterator GetChildIteratorEnd();

	protected:
		SceneNode* m_parentNode;
		Entity m_entity;
		std::vector<SceneNode*> m_children;
	};
}
