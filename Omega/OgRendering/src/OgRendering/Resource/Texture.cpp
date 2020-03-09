#include <OgRendering/Resource/Texture.h>

OgEngine::Texture::Texture()
= default;

OgEngine::Texture::Texture(const Texture & p_other)
{
	if (&p_other == this)
	{
		return;
	}

	m_hashID = p_other.m_hashID;
	m_imageSize = p_other.m_imageSize;
	m_pixels = p_other.m_pixels;
	m_width = p_other.m_width;
	m_height = p_other.m_height;
	m_mipmapLevels = p_other.m_mipmapLevels;
}

OgEngine::Texture::Texture(Texture && p_other) noexcept
{
	m_hashID = p_other.m_hashID;
	m_imageSize = p_other.m_imageSize;
	m_pixels = p_other.m_pixels;
	m_width = p_other.m_width;
	m_height = p_other.m_height;
	m_mipmapLevels = p_other.m_mipmapLevels;
}

void OgEngine::Texture::SetHashID(const uint64_t p_hashID)
{
	m_hashID = p_hashID;
}

stbi_uc* OgEngine::Texture::Pixels() const
{
	return m_pixels;
}

VkDeviceSize OgEngine::Texture::ImageSize() const
{
	return m_imageSize;
}

uint32_t OgEngine::Texture::Width() const
{
	return m_width;
}

uint32_t OgEngine::Texture::Height() const
{
	return m_height;
}

uint64_t OgEngine::Texture::HashID() const
{
	return m_hashID;
}

uint32_t OgEngine::Texture::MipmapLevels() const
{
	return m_mipmapLevels;
}

OgEngine::Texture& OgEngine::Texture::operator=(const Texture & p_other)
{
	if (&p_other == this)
		return *this;

	m_hashID = p_other.m_hashID;
	m_imageSize = p_other.m_imageSize;
	m_pixels = p_other.m_pixels;
	m_width = p_other.m_width;
	m_height = p_other.m_height;
	m_mipmapLevels = p_other.m_mipmapLevels;

	return *this;
}

OgEngine::Texture& OgEngine::Texture::operator=(Texture && p_other) noexcept
{
	m_hashID = p_other.m_hashID;
	m_imageSize = p_other.m_imageSize;
	m_pixels = p_other.m_pixels;
	m_width = p_other.m_width;
	m_height = p_other.m_height;
	m_mipmapLevels = p_other.m_mipmapLevels;

	return *this;
}

OgEngine::Texture::~Texture()
{
	stbi_image_free(m_pixels);
}

void OgEngine::Texture::FillData(stbi_uc * p_pixels, const uint32_t p_width, const uint32_t p_height,
	const uint32_t p_mipmapLevels)
{
	m_pixels = p_pixels;
	m_width = p_width;
	m_height = p_height;
	m_mipmapLevels = p_mipmapLevels;

	m_imageSize = m_width * m_height * 4u;
}

void OgEngine::Texture::FillData(const std::shared_ptr<Texture> & p_other)
{
	if (p_other != nullptr)
	{
		m_imageSize = p_other->m_imageSize;
		m_pixels = p_other->m_pixels;
		m_width = p_other->m_width;
		m_height = p_other->m_height;
		m_mipmapLevels = p_other->m_mipmapLevels;
	}
}
