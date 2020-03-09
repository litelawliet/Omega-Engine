#pragma once

#include <cmath>
#include <type_traits>
#include <algorithm>

namespace GPM::Tools
{
	template <typename T>
	inline T Utils::ToRadians(const T p_angle)
	{
		static_assert(std::is_arithmetic<T>::value, "ToRadians function should only be used with arithmetic types");

		return p_angle * (static_cast<T>(M_PI) / T(180.0));
	}

	template <typename T>
	inline T Utils::ToDegrees(const T p_angle)
	{
		static_assert(std::is_arithmetic<T>::value, "ToDegrees function should only be used with arithmetic types");

		return p_angle * (T(180.0) / static_cast<T>(M_PI));
	}

	template <typename T>
	inline T Utils::Lerp(const T p_a, const T p_b, const T p_alpha)
	{
		static_assert(std::is_arithmetic<T>::value, "Lerp function should only be used with arithmetic types");

		if (p_alpha >= T(0.0) && p_alpha <= T(1.0))
			return p_a + (p_b - p_a) * p_alpha;

		if (p_alpha < T(0.0))
			return p_a;

		return p_b;
	}

	inline int Utils::GreatestCommonDivider(const int p_a, const int p_b)
	{
		for (int divisor = std::min(p_a, p_b); divisor > 0; divisor--) {
			if (0 == p_a % divisor && 0 == p_b % divisor) {
				return divisor;
			}
		}

		return p_b;
	}

	inline int Utils::LeastCommonMultiple(const int p_a, const int p_b)
	{
		return p_a * p_b / GreatestCommonDivider(p_a, p_b);
	}

	template<typename T>
	inline T Utils::GetDecimalPart(const T p_value)
	{
		static_assert(std::is_arithmetic<T>::value, "Sign function should only be used with arithmetic types");
		return std::fmod(p_value, T(1.0));
	}

	inline double Utils::Sin(const double p_value)
	{
		return std::sin(p_value);
	}

	inline float Utils::SinF(const float p_value)
	{
		return std::sinf(p_value);
	}

	inline double Utils::Cos(const double p_value)
	{
		return std::cos(p_value);
	}

	inline float Utils::CosF(const float p_value)
	{
		return std::cosf(p_value);
	}

	inline double  Utils::Tan(const double p_value)
	{
		return std::tan(p_value);
	}

	inline float Utils::TanF(const float p_value)
	{
		return std::tanf(p_value);
	}

	inline double Utils::Arccos(const double p_value)
	{
		return std::acos(p_value);
	}

	inline float Utils::ArccosF(const float p_value)
	{
		return std::acosf(p_value);
	}

	inline double Utils::Arcsin(const double p_value)
	{
		return std::asin(p_value);
	}

	inline float Utils::ArcsinF(const float p_value)
	{
		return std::asinf(p_value);
	}

	inline double Utils::Arctan(const double p_value)
	{
		return std::atan(p_value);
	}

	inline float Utils::ArctanF(const float p_value)
	{
		return std::atanf(p_value);
	}

	inline double Utils::Arctan2(const double p_valueYx, const double p_valueXx)
	{
		return std::atan2(p_valueYx, p_valueXx);
	}

	inline float Utils::Arctan2F(const float p_valueYx, const float p_valueXx)
	{
		return std::atan2f(p_valueYx, p_valueXx);
	}

	template <typename T>
	inline T Utils::Sign(const T p_value)
	{
		static_assert(std::is_arithmetic<T>::value, "Sign function should only be used with arithmetic types");
		return (p_value > static_cast<T>(0)) ? static_cast<T>(1) : ((p_value < static_cast<T>(0)) ? static_cast<T>(-1) : static_cast<T>(0));
	}

	template<typename T>
	inline T Utils::Pow(const T p_value, const int p_exp)
	{
		static_assert(std::is_arithmetic<T>::value, "The value to pow must be arithmetic");
		return static_cast<T>(std::pow(p_value, p_exp));
	}

	template<typename T>
	inline T Utils::Pow(const T p_value, const float p_exp)
	{
		static_assert(std::is_arithmetic<T>::value, "The value to pow must be arithmetic");
		return static_cast<T>(std::pow(p_value, p_exp));
	}

	template <typename T>
	inline T Utils::SquareRoot(const T p_value)
	{
		static_assert(std::is_arithmetic<T>::value, "The value to root must be arithmetic");
		return static_cast<T>(std::sqrt(p_value));
	}

	template <typename T>
	inline T Utils::SquareRootF(const T p_value)
	{
		static_assert(std::is_arithmetic<T>::value, "The value to root must be arithmetic");
		return static_cast<T>(std::sqrtf(p_value));
	}

	template<typename T>
	inline T Utils::Root(const T p_value, const float p_nRoot)
	{
		static_assert(std::is_arithmetic<T>::value, "The value to root must be arithmetic");
		return static_cast<T>(std::pow(p_value, 1 / p_nRoot));
	}

	template<typename T>
	inline T Utils::Abs(const T p_value)
	{
		static_assert(std::is_arithmetic<T>::value, "The value to Abs must be arithmetic");
		return static_cast<T>(std::abs(p_value));
	}
}
