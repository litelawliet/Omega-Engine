#pragma once

namespace GPM::Tools
{
	template <typename T>
	T Random::GenerateInt(const T p_min, const T p_max)
	{
		static_assert(std::is_arithmetic<T>::value && std::is_integral<T>::value, "The value to GenerateInt must be an integer");
		std::uniform_int_distribution<T> distribution(p_min, p_max);
		return { distribution(s_generator) };
	}

	template <typename T>
	T Random::GenerateFloat(const T p_min, const T p_max)
	{
		static_assert(std::is_arithmetic<T>::value && std::is_integral<T>::value, "The value to GenerateInt must be an integer");
		std::uniform_real_distribution<T> distribution(p_min, p_max);
		return { distribution(s_generator) };
	}
}
