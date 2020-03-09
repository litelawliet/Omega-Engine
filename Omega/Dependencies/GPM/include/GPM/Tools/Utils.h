#pragma once

namespace GPM::Tools
{
	/**
	 * @brief PI alias for maths
	 */
	constexpr long double M_PI = 3.141592653589793238462643383279502884L;

	/**
	* Return the square root of a numeric value
	* @param n
	*/
	/*double inline __declspec (naked) __fastcall FastSquareRoot(const double n)
	{
		__asm fld qword ptr[esp + 4]
			__asm fsqrt
		__asm ret 8
	}*/
	
	/**
	 * Utils class provides some simple mathematics tools, such as operations, pow, root, trigonometry stuffs...
	 */
	class Utils final
	{
	public:
		Utils() = delete;

		/**
		 * @brief Convert the given angle to radians
		 * @param p_angle (In degrees)
		 * @return The angle in radians
		 */
		template <typename T>
		static T ToRadians(const T p_angle);


		/**
		 * @brief Convert the given angle to degrees in float precision
		 * @param p_angle (In radians)
		 * @return The result in degrees
		 */
		template <typename T>
		static T ToDegrees(const T p_angle);

		/**
		 * @brief Linearly interpolates between two values
		 * @param p_a Start value
		 * @param p_b End value
		 * @param p_alpha Coefficient
		 * @return The interpolated value
		 */
		template <typename T>
		static T Lerp(const T p_a, const T p_b, const T p_alpha);

		/**
		 * @brief Return the pow of a numeric value with an integer exponent
		 * @param p_value The value
		 * @param p_exp The power
		 * @return The result of power
		 */
		template<typename T>
		static T Pow(const T p_value, const int p_exp);

		/**
		* @brief Return the pow of a numeric value with a float exponent
		* @param p_value The value
		* @param p_exp The power
		* @return The result of power
		*/
		template<typename T>
		static T Pow(const T p_value, const float p_exp);


		/**
		* @brief Return the square root of a numeric value
		* @param p_value Value
		* @return The square root
		*/		
		template<typename T>
		static T SquareRoot(const T p_value);
		
		/**
		* @brief Return the square root of a numeric value with float precision
		* @param p_value The
		* @return The result square root
		*/
		template<typename T>
		static T SquareRootF(const T p_value);

		/**
		 * @brief Return the root of a numeric value
		 * @param p_value Value
		 * @param p_nRoot Root
		 * @return The root
		 */
		template<typename T>
		static T Root(const T p_value, const float p_nRoot);

		/**
		 * @brief Return the greatest common divider of the two given integers
		 * @param p_a Left value
		 * @param p_b Right value
		 * @return The greatest common divider
		 */
		static int GreatestCommonDivider(const int p_a, const int p_b);

		/**
		 * @brief Return the smallest common divider of the two given integers
		 * @param p_a Left value
		 * @param p_b Right value
		 * @return The least common multiple
		 */
		static int LeastCommonMultiple(const int p_a, const int p_b);

		/**
		 * @brief Return the decimal part of a float as an integer
		 * @param p_value The value from which we take the decimal part
		 * @return The decimal part
		 */
		template<typename T>
		static T GetDecimalPart(const T p_value);

		/**
		 * @brief Return the absolute value of a numeric value
		 * @param p_value The value to make absolute
		 * @return The absolute value
		 */
		template<typename T>
		static T Abs(const T p_value);

		/**
		 * @brief Mathematics sinus implementation
		 * @param p_value Value
		 * @return The result in radians
		 */
		static double Sin(const double p_value);

		/**
		* @brief Mathematics sinus implementation with float precision
		* @param p_value Value
		* @return The result in radians
		*/
		static float SinF(const float p_value);

		/**
		 * @brief Mathematics co-sinus implementation
		 * @param p_value Value
		 * @return The result in radians
		 */
		static double Cos(const double p_value);

		/**
		* @brief Mathematics co-sinus implementation with float precision
		* @param p_value Value
		* @return The result in radians
		*/
		static float CosF(const float p_value);

		/**
		 * @brief Mathematics tangent implementation
		 * @param p_value Value
		 * @return The result in radians
		 */
		static double Tan(const double p_value);

		/**
		 * @brief Mathematics tangent implementation with float precision
		 * @param p_value Value
		 * @return The result in radians
		 */
		static float TanF(const float p_value);
		
		/**
		 * @biref Mathematics arc co-sinus implementation
		 * @param p_value Value
		 * @return The result in radians
		 */
		static double Arccos(const double p_value);

		/**
		 * @brief Mathematics arc co-sinus implementation with float precision
		 * @param p_value Value
		 * @return The result in radians
		 */
		static float ArccosF(const float p_value);

		/**
		 * @brief Mathematics arc sinus implementation
		 * @param p_value Value
		 * @return The result in radians
		 */
		static double Arcsin(const double p_value);

		/**
		 * Mathematics arc sinus implementation with float precision
		 * @param p_value Value
		 * @return The result in radians
		 */
		static float ArcsinF(const float p_value);

		/**
		 * @brief Mathematics arc tangent implementation
		 * @param p_value Value
		 * @return The result in radians
		 */
		static double Arctan(const double p_value);

		/**
		 * @brief Mathematics arc tangent implementation with float precision
		 * @param p_value Value
		 * @return The result in radians
		 */
		static float ArctanF(const float p_value);

		/**
		* @brief Mathematics arc tangent implementation
		* @param p_valueYx Left value
		* @param p_valueXx Right value
		* @return The result in radians
		*/
		static double Arctan2(const double p_valueYx, const double p_valueXx);

		/**
		* @brief Mathematics arc tangent implementation with float precision
		* @param p_valueYx
		* @param p_valueXx
		* @return The result in radians
		*/
		static float Arctan2F(const float p_valueYx, const float p_valueXx);

		/**
		* @brief Return the sign of the parameter
		* @param p_value The value to check
		* @return The sign of p_value of type T
		*/
		template <typename T>
		static T Sign(const T p_value);
	};
}

#include <GPM/Tools/Utils.inl>