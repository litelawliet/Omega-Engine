#include <OgRendering/Managers/Services/TextureService.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
OgEngine::Services::TextureService::TextureService()
{
}

OgEngine::Services::TextureService::~TextureService()
{
	WaitForAll();
	m_workerToTexture.clear();
	m_textures.clear();
}

void OgEngine::Services::TextureService::Add(std::string_view p_filePath)
{
	const std::string_view fileName{p_filePath.data() + (p_filePath.find_last_of('/') + 1)};

	if (m_textures.find(fileName.data()) != m_textures.end())
	{
		std::cout << "Warning: The file '" << fileName << "' already exist in memory, loading is discarded.\n";
		return;
	}
	m_textures.insert({fileName.data(), std::make_shared<Texture>()});

	m_textures.at(fileName.data())->SetHashID(m_hashValueFromName(p_filePath.data()));

	m_workerToTexture.emplace_back(std::make_pair<uint64_t, std::string>(m_pool.WorkersInUse(), fileName.data()));

	m_pool.AddTask(&TextureService::MultithreadedLoading, this, p_filePath);
}

std::shared_ptr<OgEngine::Texture> OgEngine::Services::TextureService::Get(std::string_view p_textureName) const
{
	if (m_textures.find(p_textureName.data()) != m_textures.end())
		return m_textures.at(p_textureName.data());

	return nullptr;
}

inline void OgEngine::Services::TextureService::WaitForResource(std::string_view p_textureName)
{
	const auto& pairFound = std::
			find_if(m_workerToTexture.begin(), m_workerToTexture.end(),
			        [p_textureName](const std::pair<uint64_t, std::string>& element)
			        {
				        return element.second == p_textureName.data();
			        });

	if (pairFound != m_workerToTexture.end())
	{
		m_pool.WaitForWorker(pairFound->first);
		m_workerToTexture.erase(pairFound);
	}
	else
		std::cerr << "Couldn't find " << p_textureName.data() <<
				", waiting for the Resource skipped.\nThe resource might be already into memory or the file name is misspelled.\n";
}

std::vector<OgEngine::Texture*>& OgEngine::Services::TextureService::GetAllTextures()
{
	m_texturesRefs.resize(m_textures.size());

	auto i = 0;
	for (const auto& element : m_textures)
	{
		m_texturesRefs[i] = element.second.get();
		++i;
	}

	return m_texturesRefs;
}

void OgEngine::Services::TextureService::MultithreadedLoading(std::string_view p_filePath)
{
	const std::string_view fileName{ p_filePath.data() + (p_filePath.find_last_of('/') + 1) };
	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_set_flip_vertically_on_load(true);

	// Check if we eitheir have a relative or full path
	stbi_uc* pixels = nullptr;
	const size_t index = p_filePath.find_first_of('/');
	const std::string_view lectorName = p_filePath.data() + index;
	const size_t lectorID = lectorName.find(":");
	if (index != std::string::npos && index == 2 && lectorID != std::string::npos)
	{
		// Loading using full path
		FILE* fp = nullptr;
		errno_t err;
		if ((err = fopen_s(&fp, p_filePath.data(), "r")) == 0)
		{
			pixels = stbi_load_from_file(fp, &texWidth, &texHeight, &texChannels,
				STBI_rgb_alpha);
		}

		if (fp)
		{
			fclose(fp);
		}
	}
	else
	{
		// Loading using relative path
		pixels = stbi_load(p_filePath.data(), &texWidth, &texHeight, &texChannels,
			STBI_rgb_alpha);
	}
	const uint32_t mipmaps = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	m_textures.at(fileName.data())->FillData(pixels, texWidth, texHeight, mipmaps);
	
	if (!pixels)
	{
		std::cout << "failed to load texture image named " << std::string(p_filePath.data()) << "\n";
	}
}
