#include <OgRendering/Managers/Loaders/LoaderManager.h>

inline bool OgEngine::LoaderManager::CheckValidMesh(std::string_view p_file)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(p_file.data(), aiProcessPreset_TargetRealtime_Quality);

	if (!scene)
	{
		std::cerr << "assimp error: failed to open mesh " << p_file.data() << '\n';
		return false;
	}

	return true;
}

std::string OgEngine::LoaderManager::GetFilePathExtension(const std::string& p_filename)
{
	if (p_filename.find_last_of(".") != std::string::npos)
		return p_filename.substr(p_filename.find_last_of(".") + 1);
	return "";
}
