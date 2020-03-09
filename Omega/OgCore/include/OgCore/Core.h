#pragma once
#include <OgAudio/Audio/AudioEngine.h>
#include <OgRendering/Rendering/VulkanContext.h>

#include <OgCore/SceneNode.h>

namespace OgEngine
{
	class Core
	{
	public:
		Core(const uint64_t p_width, const uint64_t p_height, const char* p_title);
		~Core();
		void Run();

	private:
		OgEngine::AudioEngine m_audioEngine;
		std::shared_ptr<OgEngine::VulkanContext> m_vulkanContext;
		SceneNode* root = nullptr;
	};
}
