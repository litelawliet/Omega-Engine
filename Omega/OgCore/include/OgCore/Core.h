#pragma once
#include <OgAudio/Audio/AudioEngine.h>
#include <OgRendering/Rendering/VulkanContext.h>
#include <OgCore/Systems/RenderingSystem.h>
#include <OgCore/SceneNode.h>

namespace OgEngine
{
	class Core
	{
	public:
		Core(const uint64_t p_width, const uint64_t p_height, const char* p_title);
		~Core();

		void Run();
		void UpdateEditor(float p_dt);
		bool LoopOnChild(OgEngine::SceneNode* node);
		void OpenAddMenu(SceneNode* node);
		void ShowInfo(OgEngine::SceneNode* node);
		void PrepareImGUIFrame();
		void DrawEditor();
        void DrawUI();
	private:
		OgEngine::AudioEngine m_audioEngine;
		std::shared_ptr<OgEngine::VulkanContext> m_vulkanContext;

		std::shared_ptr<RenderingSystem> m_renderSystem;
		SceneNode* root = nullptr;

		SceneNode* inspectorNode;
	};
}
