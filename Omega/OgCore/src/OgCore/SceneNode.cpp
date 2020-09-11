#include <OgCore/SceneNode.h>
#include <OgCore/Managers/SceneManager.h>

OgEngine::SceneNode::SceneNode(Entity p_entity)
	: m_parentNode(nullptr), m_entity(p_entity)
{
}

OgEngine::SceneNode::~SceneNode()
{
	RemoveChildren();
	SceneManager::DestroyEntity(m_entity);
}

glm::mat4 OgEngine::SceneNode::GetWorldTransform() const
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

OgEngine::SceneNode* OgEngine::SceneNode::LastChild() const
{
	if (!m_children.empty())
	{
		return m_children.back();
	}

	return nullptr;
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

OgEngine::SceneNode* OgEngine::SceneNode::GetParent() const
{
	return m_parentNode;
}

bool OgEngine::SceneNode::HasParent() const
{
	return m_parentNode != nullptr;
}

void OgEngine::SceneNode::RemoveChild(const uint64_t p_index)
{
	if (p_index < m_children.size())
	{
		delete m_children[p_index];
		m_children.erase(m_children.begin() + p_index);
	}
}

void OgEngine::SceneNode::RemoveChild(SceneNode* p_childNode)
{
	const auto it = std::find(m_children.begin(), m_children.end(), p_childNode);
	if (it != m_children.end())
	{
		m_children.erase(it);
		delete p_childNode;
		p_childNode = nullptr;
	}
}

void OgEngine::SceneNode::RemoveChildren()
{
	for (auto* it : m_children)
	{
		delete it;
	}
	m_children.clear();
}
