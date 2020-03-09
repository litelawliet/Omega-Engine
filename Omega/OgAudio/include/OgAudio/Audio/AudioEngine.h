#pragma once
#include <OgAudio/Export.h>
#include <string_view>
#include <irrKlang.h>
#include <unordered_map>

namespace OgEngine
{
	class AUDIO_API AudioEngine final
	{
	public:
		/**
		 * @brief Default constructor. Initialize and allocate the engine.
		 */
		AudioEngine();

		/**
		 * @brief Copy constructor. Initialize and allocate the engine based on the other one.
		 * @param p_other The other AudioEngine
		 */
		AudioEngine(const AudioEngine& p_other);

		/**
		 * @brief Move constructor. Initialize and allocate the engine based on the other one.
		 * @param p_other The other AudioEngine
		 */
		AudioEngine(AudioEngine&& p_other) noexcept;

		/**
		 * @brief Destructor. Deallocate the audio engine.
		 */
		~AudioEngine();

		/**
		 * @brief Copy operator.
		 * @param p_other The other AudioEngine
		 * @return Return the SoundSource after adding it in the audio engine.
		 */
		AudioEngine& operator=(const AudioEngine& p_other) = default;

		/**
		 * @brief Move operator.
		 * @param p_other The other AudioEngine
		 * @return Return the SoundSource after adding it in the audio engine.
		 */
		AudioEngine& operator=(AudioEngine&& p_other) noexcept;

		/**
		 * @brief Add a sound in the engine using the file path.
		 * @param p_filePath Path of the sound
		 * @return Return the SoundSource after adding it in the audio engine.
		 */
		irrklang::ISoundSource* AddSoundSource(const char* p_filePath);

		/**
		 * @brief Play a sound in 2D and handle the sound internally with path, filename and sound pointer.
		 * @param p_soundName Name of the sound
		 * @param p_loop Loop the sound
		 * @param p_startPaused Start or pause the sound at beginning
		 * @param p_track Track the sound or not
		 */
		void Play2D(
			const std::string_view& p_soundName,
			const bool p_loop = false,
			const bool p_startPaused = false,
			const bool p_track = false);

		/**
		 * @brief Play a sound in 3D at a position and handle the sound internally with path, filename and sound pointer.
		 * @param p_soundName Name of the sound
		 * @param p_x Position in x of the sound
		 * @param p_y Position in y of the sound
		 * @param p_z Position in z of the sound
		 * @param p_loop Loop the sound
		 * @param p_startPaused Start or pause the sound at beginning
		 * @param p_track Track the sound or not
		 */
		void Play3D(
			const std::string_view& p_soundName,
			const float p_x,
			const float p_y,
			const float p_z,
			const bool p_loop = false,
			const bool p_startPaused = false,
			const bool p_track = false);

		/**
		 * @brief Stop all playing sounds.
		 */
		void StopAllSounds() const;

		/**
		 * @brief Stop the playing sound.
		 * @param p_soundName Name of the sound
		 */
		void StopSound(const std::string_view& p_soundName) const;

		/**
		 * @brief Change the position of the listener in the engine.
		 * @param p_position New position of the listener in the engine
		 * @param p_direction New direction of the listener in the engine
		 */
		void SetListenerPosition(
			const irrklang::vec3df& p_position = irrklang::vec3df{ 1.0, 1.0, 1.0 },
			const irrklang::vec3df& p_direction = irrklang::vec3df{ 0.0, 0.0, 1.0 }) const;

		/**
		 * @brief Change the position of a 3D sound.
		 * @param p_soundName Name of the sound to play
		 * @param p_position New position where to play the sound
		 * @note This method can only works on 3D sounds, to ensure a sound is a 3D sound
		 * just make sure you set the sound as a track (true) in the Play3D method.
		 */
		void SetMusicPosition(
			const std::string_view& p_soundName,
			const irrklang::vec3df& p_position = irrklang::vec3df{ 0.0, 0.0, 0.0 });

		/**
		 * @brief Pause the sound if playing.
		 * @param p_soundName Name of the sound to pause
		 */
		void PauseSound(const std::string_view& p_soundName);

		/**
		 * @brief Resume the sound if not playing.
		 * @param p_soundName Name of the sound to resume
		 */
		void ResumeSound(const std::string_view& p_soundName);

		/**
		 * @brief Set the master volume of the engine. All sounds are multiplied to this volume.
		 * @param p_volume New volume value, has to be between 0 and 1
		 */
		void SetMasterVolume(const float p_volume) const;

		/**
		 * @brief Set the sound volume. The sound is multiplied to the master volume of the engine.
		 * @param p_soundName Sound name
		 * @param p_volume New volume value, has to be between 0 and 1
		 */
		void SetSoundVolume(const std::string_view& p_soundName, const float p_volume) const;

		/**
		 * @brief Set the sound speed.
		 * @param p_soundName Sound name
		 * @param p_speed New speed value, has to be between 0 and 1
		 */
		void SetSoundSpeed(const std::string_view& p_soundName, const float p_speed) const;

		/**
		 * @brief Get the volume of the sound.
		 * @param p_soundName Sound name
		 * @return Return the volume of the sound
		 */
		float SoundVolume(const std::string_view& p_soundName) const;

		/**
		 * @brief Get the speed of the sound.
		 * @param p_soundName Sound name
		 * @return Return the speed of the sound
		 */
		float SoundSpeed(const std::string_view& p_soundName) const;

		/**
		 * @brief Get the length of the sound.
		 * @param p_soundName Sound name
		 * @return Return the length of the sound
		 */
		uint32_t SoundLength(const std::string_view& p_soundName) const;

	private:
		irrklang::ISoundEngine* m_engine;
		std::unordered_map<std::string, const char*> m_filesPath;
		std::unordered_map<std::string, irrklang::ISound*> m_sounds;
	};
}
