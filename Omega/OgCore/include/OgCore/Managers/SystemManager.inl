#pragma once
#include <cassert>

template <typename T>
std::shared_ptr<T> OgEngine::SystemManager::RegisterSystem()
{
	const char* typeName = typeid(T).name();

	assert(m_systems.find(typeName) == m_systems.end() && "Registering system more than one.");

	// Create a pointer to the system en return it so it can be used externally
	auto system = std::make_shared<T>();
	m_systems.insert({ typeName, system });
	return system;
}

template <typename T>
void OgEngine::SystemManager::SetSignature(Signature p_signature)
{
	const char* typeName = typeid(T).name();

	assert(m_systems.find(typeName) != m_systems.end() && "System used before registered.");

	// Set the signature for this system
	m_signatures.insert({ typeName, p_signature });
}
