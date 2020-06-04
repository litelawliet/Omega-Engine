#include <OgAudio/Audio/AudioEngine.h>
#include <iostream>

OgEngine::AudioEngine::AudioEngine()
	: m_engine{ nullptr }
{
	m_engine = irrklang::createIrrKlangDevice();
}

OgEngine::AudioEngine::AudioEngine(const AudioEngine& p_other)
{
	m_engine = p_other.m_engine;
	m_filesPath = p_other.m_filesPath;
	m_sounds = p_other.m_sounds;
}

OgEngine::AudioEngine::AudioEngine(AudioEngine&& p_other) noexcept
{
	try
	{
		m_engine = p_other.m_engine;
		m_filesPath = p_other.m_filesPath;
		m_sounds = p_other.m_sounds;
	}
	catch (const std::exception & exception)
	{
		std::cerr << exception.what();
	}
}

OgEngine::AudioEngine::~AudioEngine()
{
	if (m_engine)
	{
		m_sounds.clear();
		m_filesPath.clear();
		m_engine->removeAllSoundSources();
		m_engine->drop();
		m_engine = nullptr;
	}
}

OgEngine::AudioEngine& OgEngine::AudioEngine::operator=(OgEngine::AudioEngine&& p_other) noexcept
{
	try
	{
		m_engine = p_other.m_engine;
		m_filesPath = p_other.m_filesPath;
		m_sounds = p_other.m_sounds;

	}
	catch (const std::exception & exception)
	{
		std::cerr << exception.what();
	}

	return *this;
}

irrklang::ISoundSource* OgEngine::AudioEngine::AddSoundSource(const char* p_filePath)
{
	if (m_engine)
		m_filesPath.try_emplace(strrchr(p_filePath, '/') + 1, p_filePath);

	return nullptr;
}

void OgEngine::AudioEngine::Play2D(
	const std::string_view& p_soundName,
	const bool p_loop,
	const bool p_startPaused,
	const bool p_track)
{
	if (m_engine)
	{
		const auto& occurence = m_filesPath.find(p_soundName.data());
		if (occurence != m_filesPath.cend())
		{
			irrklang::ISound* soundPtr = m_engine->play2D(
				occurence->second,
				p_loop,
				p_startPaused,
				p_track
			);

			if (soundPtr)
				m_sounds.try_emplace(p_soundName.data(), soundPtr);
		}
	}
}

void OgEngine::AudioEngine::Play3D(
	const std::string_view& p_soundName,
	const float p_x,
	const float p_y,
	const float p_z,
	const bool p_loop,
	const bool p_startPaused,
	const bool p_track)
{
	if (m_engine)
	{
		const auto& occurence = m_filesPath.find(p_soundName.data());
		if (occurence != m_filesPath.cend())
		{
			irrklang::ISound* soundPtr = m_engine->play3D(
				occurence->second,
				irrklang::vec3df(p_x, p_y, p_z),
				p_loop,
				p_startPaused,
				p_track
			);

			if (soundPtr)
				m_sounds.try_emplace(p_soundName.data(), soundPtr);
		}
	}
}

void OgEngine::AudioEngine::StopAllSounds() const
{
	if (m_engine)
		m_engine->stopAllSounds();
}

void OgEngine::AudioEngine::StopSound(const std::string_view& p_soundName) const
{
	if (m_engine)
	{
		m_engine->stopAllSoundsOfSoundSource(
			m_engine->getSoundSource(m_filesPath.at(p_soundName.data()),
				false)
		);
	}
}

void OgEngine::AudioEngine::SetListenerPosition(const irrklang::vec3df& p_position, const irrklang::vec3df& p_direction) const
{
	if (m_engine)
	{
		m_engine->setListenerPosition(p_position, p_direction);
	}
}

void OgEngine::AudioEngine::SetMusicPosition(const std::string_view& p_soundName, const irrklang::vec3df& p_position)
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				iterator->second->setPosition(p_position);
		}
	}
}

void OgEngine::AudioEngine::PauseSound(const std::string_view& p_soundName)
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				iterator->second->setIsPaused(true);
		}
	}
}

void OgEngine::AudioEngine::ResumeSound(const std::string_view& p_soundName)
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				iterator->second->setIsPaused(false);
		}
	}
}

void OgEngine::AudioEngine::SetMasterVolume(const float p_volume) const
{
	if (m_engine)
	{
		m_engine->setSoundVolume(p_volume);
	}
}

void OgEngine::AudioEngine::SetSoundVolume(const std::string_view& p_soundName, const float p_volume) const
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				iterator->second->setVolume(p_volume);
		}
	}
}

void OgEngine::AudioEngine::SetSoundSpeed(const std::string_view& p_soundName, const float p_speed) const
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				iterator->second->setPlaybackSpeed(p_speed);
		}
	}
}

float OgEngine::AudioEngine::SoundVolume(const std::string_view& p_soundName) const
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				return iterator->second->getVolume();
		}
	}

	return 1.0f;
}

float OgEngine::AudioEngine::SoundSpeed(const std::string_view& p_soundName) const
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				return iterator->second->getPlaybackSpeed();
		}
	}

	return 1.0f;
}

uint32_t OgEngine::AudioEngine::SoundLength(const std::string_view& p_soundName) const
{
	if (m_engine)
	{
		const auto& iterator = m_sounds.find(p_soundName.data());
		if (iterator != m_sounds.cend())
		{
			if (iterator->second)
				return iterator->second->getPlayLength();
		}
	}

	return -1;
}