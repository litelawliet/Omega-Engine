#pragma once
#include <OgRendering/Export.h>

#include <array>
#include <cstddef>
#include <functional>
#include <vulkan/vulkan.h>

#include <GPM/GPM.h>

namespace OgEngine
{
	struct RENDERING_API Vertex {
		alignas(alignof(GPM::Vector3F)) GPM::Vector3F position{};
		alignas(alignof(GPM::Vector3F)) GPM::Vector3F normal{};
		alignas(alignof(GPM::Vector3F)) GPM::Vector3F tangent{};
		alignas(alignof(GPM::Vector2F)) GPM::Vector2F texCoord{};
		alignas(alignof(int)) int dummy{ 0 };

		Vertex();
		Vertex(const Vertex& p_other);
		Vertex(Vertex&& p_other) noexcept;
		~Vertex() = default;

		static VkVertexInputBindingDescription getBindingDescription();

		static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();

		bool operator<(const Vertex& p_other) const;
		bool operator==(const Vertex& p_other) const;
		Vertex& operator=(const Vertex& p_other);
		Vertex& operator=(Vertex&& p_other) noexcept;
	};

	class VertexHashFunction {
	public:

		static void hash_combine(size_t& seed, const size_t& hash)
		{
			seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		size_t operator() (const Vertex& p_vec) const noexcept
		{
			const auto hasher = std::hash<float>{};
			const auto hashed_x = hasher(p_vec.position.x);
			const auto hashed_y = hasher(p_vec.position.y);
			const auto hashed_z = hasher(p_vec.position.z);
			const auto hashed_tangent_x = hasher(p_vec.tangent.x);
			const auto hashed_tangent_y = hasher(p_vec.tangent.y);
			const auto hashed_tangent_z = hasher(p_vec.tangent.z);
			const auto hashed_texcoord_x = hasher(p_vec.texCoord.x);
			const auto hashed_texcoord_y = hasher(p_vec.texCoord.y);
			const auto hashed_normal_x = hasher(p_vec.normal.x);
			const auto hashed_normal_y = hasher(p_vec.normal.y);
			const auto hashed_normal_z = hasher(p_vec.normal.z);

			size_t seed = 0;
			hash_combine(seed, hashed_x);
			hash_combine(seed, hashed_y);
			hash_combine(seed, hashed_z);
			hash_combine(seed, hashed_texcoord_x);
			hash_combine(seed, hashed_texcoord_y);
			hash_combine(seed, hashed_normal_x);
			hash_combine(seed, hashed_normal_y);
			hash_combine(seed, hashed_normal_z);
			hash_combine(seed, hashed_tangent_x);
			hash_combine(seed, hashed_tangent_y);
			hash_combine(seed, hashed_tangent_z);
			return seed;
		}
	};

	inline bool operator<(const Vector3F& p_left, const Vector3F& p_right)
	{
		return p_left.x < p_right.x || p_left.y < p_right.y || p_left.z < p_right.z;
	}

	inline bool operator<(const Vector2F& p_left, const Vector2F& p_right)
	{
		return p_left.x < p_right.x || p_left.y < p_right.y;
	}
}