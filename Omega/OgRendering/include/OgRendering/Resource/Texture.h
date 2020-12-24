#pragma once
#include <OgRendering/Export.h>

#include <cstdint>
#include <memory>
#include <OgRendering/Utils/volk.h>

#include <OgRendering/Rendering/stb_image.h>

namespace OgEngine
{
	class RENDERING_API Texture final
	{
	public:
		Texture();
		Texture(const Texture& p_other);
		Texture(Texture&& p_other) noexcept;
		~Texture();

		void FillData(stbi_uc* p_pixels, const uint32_t p_width, const uint32_t p_height,
			const uint32_t p_mipmapLevels);
		void FillData(const std::shared_ptr<Texture>& p_other);

		void SetHashID(const uint64_t p_hashID);

		[[nodiscard]] stbi_uc* Pixels() const;
		[[nodiscard]] VkDeviceSize ImageSize() const;
		[[nodiscard]] uint32_t Width() const;
		[[nodiscard]] uint32_t Height() const;
		[[nodiscard]] uint64_t HashID() const;
		[[nodiscard]] uint32_t MipmapLevels() const;

		Texture& operator=(const Texture& p_other);
		Texture& operator=(Texture&& p_other) noexcept;

	private:
		uint64_t m_hashID{};
		VkDeviceSize m_imageSize{};
		stbi_uc* m_pixels{ nullptr };
		uint32_t m_width{};
		uint32_t m_height{};
		uint32_t m_mipmapLevels{};
	};
}
