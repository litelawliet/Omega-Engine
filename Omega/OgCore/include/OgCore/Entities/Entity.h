#pragma once
#include <OgCore/Export.h>
#include <cstdint>
#include <bitset>

namespace OgEngine
{
	using Entity = std::uint64_t;

	constexpr Entity MAX_ENTITIES = 5000u;

	using ComponentType = std::uint8_t;

	constexpr ComponentType MAX_COMPONENTS = 32;

	using Signature = std::bitset<MAX_COMPONENTS>;
}
