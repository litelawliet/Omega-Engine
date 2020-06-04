Copyright Accelit.

I. Introduction
Omega is a 6 months game engine project made during the end of our studies.
It is written by Victor JORE and Thibaut PONCHON between January 2020 and June 2020.

This Game Engine is powered by Vulkan 1.2.131.2 and made for performance and realistic rendering. As such, Raytracing is part of the rendering capabilities of this game engine using a Turing GPU.
Obviously this game engine is able to do classic rendering with the rasterization being the default rendering pipeline at launch.

It uses our own math library, assimp, irrklang, React3dPhysics, stb_image, GLFW.

II. Releases note
SPRINT3 release note:
- Rasterizer
	ECS can now add entities into the scene with components. All components are not done yet but can be easily added in the future thanks to a flexible ECS architecture.
	Rasterizer pipeline can use texture and move objects over time, and create buffers and texture when needed. Some functionnality are in testing as removing and adding entities in run-time.
- Raytracing
	Object scene architecture has to change accordingly with the ECS texture. UV's and texture are correctly loaded, instance rendering implemented and optimized. Random undraw models issue is now fixed.
	Fix some important issues with vulkan api (leaks and crashes).
- Core
	InputManager started to be implemented.
	ResourceManager can now handle textures. 
- Dependencies
	Vulkan upgraded to 1.2.131.2, previous was 1.1.130.0.
	GLFW updated to 3.3.2, previous was 3.3.1.
- Pipeline production
	Added network compilation through local devices. Reduce up to 50% compilation time.

For testing Raytracing pipeline, please use the RT_Pipeline branch.
Also, be aware of the needed models and textures inside the ain.cpp which may cause a crash if not in the Resources folders. You can use your owns.
glm is not added in dependencies but is needed for raytracing. Please add it ;anually for now since it will be removed lately.
