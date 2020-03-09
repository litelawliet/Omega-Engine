#pragma once
#pragma warning (disable:4251)

#ifdef RENDERING_EXPORT
#define RENDERING_API __declspec(dllexport)
#else
#define RENDERING_API __declspec(dllimport)
#endif
