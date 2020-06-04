#pragma once
#pragma warning (disable:4251)

#ifdef PHYSICS_EXPORT
#define PHYSICS_API __declspec(dllexport)
#else
#define PHYSICS_API __declspec(dllimport)
#endif
