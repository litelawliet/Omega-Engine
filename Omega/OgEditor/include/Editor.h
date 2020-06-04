#pragma once
#include <filesystem>
#include <OgCore/Core.h>
#include <OgRendering/Managers/ResourceManager.h>
#include <OgCore/Managers/SceneManager.h>
#include <OgRendering/Managers/InputManager.h>
#include <OgPhysics/Physics.h>
#include <UI/imgui/imfilebrowser.h>


namespace OgEngine
{
	class Editor final
	{
	public:
		Editor(const uint64_t p_width = 1024u, const uint64_t p_height = 728u, const char* p_title = "Editor Omega");
		~Editor();
		void UpdateEditor(float p_dt);
		bool LoopOnChild(SceneNode* p_node);
		void OpenAddMenu(SceneNode* p_node);
		void ShowInfo(SceneNode* p_node);
		static void ShowFileName(const int p_spacing, const int p_y, const char* p_fileName, int& p_iteration);
		void PrepareIcons();
		void DestroyObject(SceneNode* p_node) const;
		void PrepareImGUIFrame() const;
		void DrawEditor() const;
		void RenderUI() const;
		void DisplayFileBrowser();
		ImTextureID AddIcon(const char* p_path) const;
		void Run();
		static std::string GetFileName(const std::string& p_filename);

	private:
		std::unique_ptr<OgEngine::Core> m_engine = nullptr;
		/** @brief  Pair of strings corresponding to the name and filepath respectively. */
		std::vector<std::pair<std::string, std::string>> m_texturesData;
		/** @brief  Pair of strings corresponding to the name and filepath respectively. */
		std::vector<std::pair<std::string, std::string>> m_normalMapsData;

		std::string sceneName;
		char input[32];

		ImGui::FileBrowser fileDialog;

		ImTextureID m_playIcon;
		ImTextureID m_pauseIcon;
		ImTextureID m_previewError;
		ImTextureID m_previewModel;
		ImTextureID m_previewDir;
		ImTextureID m_backButton;
		ImTextureID m_localAxisButton;
		ImTextureID m_globalAxisButton;
		ImTextureID m_trashButton;
		ImTextureID m_transformIcon;
		ImTextureID m_materialIcon;
		ImTextureID m_lightIcon;
		ImTextureID m_rigidbodyIcon;

		std::filesystem::path m_path;
		std::filesystem::path m_selectedPath;
		std::filesystem::directory_entry m_selectedFile;

		std::vector<std::string> m_previewTexturesPaths;
		std::vector<std::string> m_previewTexturesName;
		std::vector<ImTextureID> m_previewTextures;

		std::vector<std::string> m_modelNames;

		Entity currentRotationEntity;
		float currentEulers[3];

		char m_addTextureInput[128]{ "" };
		bool showAllFiles;
		bool worldRotation;

		[[nodiscard]] std::string TrimName(std::string p_name) const;
		[[nodiscard]] bool HasSpecialChar(const std::string& p_stringToCheck) const;
	};
}
