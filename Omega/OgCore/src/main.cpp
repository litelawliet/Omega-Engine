#include <iostream>
#include <OgCore/Core.h>
#include <OgRendering/Managers/ResourceManager.h>

using namespace OgEngine;

int main()
{
	try
	{
		srand(static_cast<unsigned int>(time(NULL)));
		ResourceManager::SetRaytracingLoading(false);
		ResourceManager::Add<Mesh>("Resources/models/cube.obj");
		//ResourceManager::Add<Mesh>("Resources/models/link.obj");
		ResourceManager::Add<Mesh>("Resources/models/sphere.obj");
		ResourceManager::Add<Mesh>("Resources/models/plane.obj");
		ResourceManager::Add<Mesh>("Resources/models/lucy.obj");
		ResourceManager::Add<Texture>("Resources/textures/default.png");
		ResourceManager::Add<Texture>("Resources/textures/error.png");
		ResourceManager::WaitForAll();
		Core core(1920, 1080, "Omega Editor");
		core.Run();

		return EXIT_SUCCESS;

	}
	catch (const std::exception& p_exception)
	{
		std::cerr << p_exception.what() << '\n';
		return EXIT_FAILURE;
	}
}
