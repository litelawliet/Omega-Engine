#pragma once

#include <random>

namespace GPM::Tools
{
	class Random final
	{
	private:
		static std::default_random_engine s_generator;

	public:
		Random() = delete;

		/**
		* Generate a random between two given integers (Closed interval)
		* @param p_min Minimum range
		* @param p_max Maximum range
		*/
		template<typename T>
		static T GenerateInt(const T p_min, const T p_max);

		/**
		* Generate a random between two given floats (Closed interval)
		* @param p_min Minimum range
		* @param p_max Maximum range
		*/
		template<typename T>
		static T GenerateFloat(const T p_min, const T p_max);
	};
}

#include <GPM/Tools/Random.inl>