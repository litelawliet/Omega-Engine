#include <iostream>
#include <OgAudio/Audio/AudioEngine.h>
#include <OgCore/Core.h>
#include <OgCore/Components/Transform.h>
#include <OgRendering/Managers/ResourceManager.h>
#include <OgCore/Managers/SceneManager.h>

using namespace OgEngine;

int main()
{
	try
	{

		ResourceManager::SetRaytracingLoading(false);
		ResourceManager::Add<Mesh>("Resources/models/cube.obj");
		ResourceManager::Add<Mesh>("Resources/models/link.obj");
		ResourceManager::Add<Mesh>("Resources/models/sphere.obj");
		ResourceManager::Add<Mesh>("Resources/models/plane.obj");
		ResourceManager::Add<Mesh>("Resources/models/lucy.obj");
		ResourceManager::Add<Texture>("Resources/textures/default.png");
		ResourceManager::WaitForAll();
		Core core(1280, 720, "Omega Editor");
		core.Run();

		return EXIT_SUCCESS;

	}
	catch (const std::exception& p_exception)
	{
		std::cerr << p_exception.what() << '\n';
		return EXIT_FAILURE;
	}
}
