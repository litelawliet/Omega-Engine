#pragma once
#include <OgRendering/Resource/Mesh.h>
#include "OgRendering/Managers/ResourceManager.h"

template <typename ResourceType>
inline std::shared_ptr<ResourceType> OgEngine::LoaderManager::Load(std::string_view p_file)
{
	std::cerr << "Warning: Unable to load the resource of type '" << type_name<ResourceType>() << "'.\n";
	return nullptr;
}

template <>
inline std::shared_ptr<OgEngine::Mesh> OgEngine::LoaderManager::Load<OgEngine::Mesh>(std::string_view p_file)
{
	Assimp::Importer importer;
	const aiScene* scene;
	if (ResourceManager::RaytracingLoadingEnabled())
	{
		scene = importer.ReadFile(p_file.data(), aiProcessPreset_TargetRealtime_Quality & ~aiProcess_JoinIdenticalVertices & ~aiProcess_GenSmoothNormals);
	}
	else
	{
		scene = importer.ReadFile(p_file.data(), aiProcessPreset_TargetRealtime_Quality);
	} 

	if (!scene)
	{
		std::cerr << "assimp error: failed to open mesh " << p_file.data() << '\n';
		return nullptr;
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[i];

		for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
		{
			Vertex vertex = {};

			if (mesh->HasPositions())
				vertex.position = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };
			else
				vertex.position = { 0.0f, 0.0f, 0.0f };

			if (mesh->HasNormals())
				vertex.normal = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
			else
				vertex.normal = { 1.0f, 1.0f, 1.0f };

			if (mesh->HasTextureCoords(i))
				vertex.texCoord = { mesh->mTextureCoords[i][j].x, mesh->mTextureCoords[i][j].y };
			else
				vertex.texCoord = { 1.0f, 1.0 };

			if (mesh->HasTangentsAndBitangents())
			{
				vertex.tangent = { mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z };
				//vertex.bitangent = { mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z };
			}
			else
			{
				vertex.tangent = { 1.0f, 1.0f, 1.0f };
				//vertex.bitangent = { 1.0f, 1.0f, 1.0f };
			}

			vertices.emplace_back(vertex);
		}

		for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
		{
			for (unsigned int k = 0; k < mesh->mFaces[j].mNumIndices; ++k)
			{
				indices.emplace_back(mesh->mFaces[j].mIndices[k]);
			}
		}
	}

	return std::make_shared<Mesh>(vertices, indices);
}