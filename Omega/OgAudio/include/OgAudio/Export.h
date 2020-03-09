#pragma once
#pragma warning (disable:4251)

#ifdef AUDIO_EXPORT
#define AUDIO_API __declspec(dllexport)
#else
#define AUDIO_API __declspec(dllimport)
#endif
