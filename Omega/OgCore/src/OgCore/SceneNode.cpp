#include <OgCore/SceneNode.h>
#include <OgCore/Managers/SceneManager.h>

OgEngine::SceneNode::SceneNode(Entity p_entity)
	: m_parentNode(nullptr), m_entity(p_entity)
{
}

OgEngine::SceneNode::~SceneNode()
{
	for (auto& i : m_children)
	{
		SceneManager::DestroyEntity(i->GetEntity());
		delete i;
	}
}

Matrix4F OgEngine::SceneNode::GetWorldTransform() const
{
	return SceneManager::GetComponent<Transform>(m_entity).worldMatrix;
}

OgEngine::Entity OgEngine::SceneNode::GetEntity() const
{
	return m_entity;
}

void OgEngine::SceneNode::AddChild(SceneNode* p_childNode)
{
	m_children.push_back(p_childNode);
	p_childNode->m_parentNode = this;
}

OgEngine::SceneNode* OgEngine::SceneNode::GetChild(const uint64_t p_childIndex)
{
	assert(p_childIndex < m_children.size() && "GetChild out of bound");

	return m_children[p_childIndex];
}

std::vector<OgEngine::SceneNode*>& OgEngine::SceneNode::GetChildren()
{
	return m_children;
}

uint64_t OgEngine::SceneNode::ChildCount() const
{
	return m_children.size();
}

void OgEngine::SceneNode::Update(float p_dt)
{
	if (m_parentNode)
	{ // This node has a parent ...
		auto& parentTransform = SceneManager::GetComponent<Transform>(m_parentNode->m_entity);
		auto& currentTransform = SceneManager::GetComponent<Transform>(m_entity);
		currentTransform.SetWorldMatrix(parentTransform.worldMatrix * currentTransform.localMatrix);
	}
	else
	{ // Root node , world transform is local transform !
		auto& currentTransform = SceneManager::GetComponent<Transform>(m_entity);
		currentTransform.SetWorldMatrix(currentTransform.localMatrix);
	}
	for (auto& i : m_children)
	{
		i->Update(p_dt);
	}
}

std::vector<OgEngine::SceneNode*>::const_iterator OgEngine::SceneNode::GetChildIteratorStart()
{
	return m_children.begin();
}

std::vector<OgEngine::SceneNode*>::const_iterator OgEngine::SceneNode::GetChildIteratorEnd()
{
	return m_children.end();
}
