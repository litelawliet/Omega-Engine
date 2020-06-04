#pragma once
#include <sstream>
#include <string>

namespace GPM
{
	/**
	 * Vector structure containing x, y coordinates
	 */
	template<typename T>
	struct Vector2
	{
		static_assert(std::is_arithmetic<T>::value, "Vector2 should only be used with arithmetic types");

		T x;
		T y;

		/**
		 * @brief Default destructor
		 */
		~Vector2() = default;
		
		/**
		 * @brief Constructor with parameters
		 * @param p_x x coordinate
		 * @param p_y y coordinate
		 */
		constexpr Vector2(const T p_x = 0, const T p_y = 0);

		/**
		 * @brief Copy Constructor
		 * @param p_other The vector to construct from
		 */
		constexpr Vector2(const Vector2<T>& p_other);

		/**
		 * @brief Move Constructor
		 * @param p_other The vector to construct from
		 */
		constexpr Vector2(Vector2<T>&& p_other) noexcept;

		/**
		 * @brief Set this Vector's values x, y to p_x, p_y
		 * @param p_x x coordinate
		 * @param p_y y coordinate
		 * */
		constexpr void Set(T p_x, T p_y);


		/**
		 * @brief Creates a string representing this Vector
		 * @return this Vector in string format
		 */
		[[nodiscard]] constexpr std::string ToString() const;

		static const Vector2<T> zero;
		static const Vector2<T> up;
		static const Vector2<T> right;

#pragma region Member Operator Overloads

		/**
		 * @brief Return true if the two vectors are identical
		 * @param p_other The vector used for the checkup
		 * @return True or false
		 */
		constexpr bool operator==(const Vector2<T>& p_other) const;

		/**
		* @brief Return false if the two vectors are identical
		* @param p_other The vector used for the checkup
		* @return True or false
		*/
		constexpr bool operator!=(const Vector2<T>& p_other) const;

		/**
		 * @brief Overload = operator by copy
		 * @param p_other The vector to construct from
		 * @return The current vector modified
		 */
		constexpr Vector2<T>& operator=(const Vector2<T>& p_other);

		/**
		 * @brief Overload = operator by copy
		 * @param p_other The vector to construct from
		 * @return The current vector modified
		 */
		template <typename U>
		constexpr Vector2<T>& operator=(const Vector2<U>& p_other);

		/**
		 * @brief Overload = operator by move
		 * @param p_other The vector to construct from
		 * @return The current vector modified
		 */
		inline constexpr GPM::Vector2<T>& operator=(Vector2<T>&& p_other) noexcept;

		/**
		 * @brief Overload = operator by move
		 * @param p_other The vector to construct from
		 * @return The current vector modified
		 */
		template <typename U>
		inline constexpr GPM::Vector2<T>& operator=(Vector2<U>&& p_other);

#pragma endregion

#pragma region Vector Operations
#pragma region Non-Static

		/**
		 * @brief Modifies this vector to make its norm (magnitude) == 1
		 */
		constexpr void Normalize();

		/**
		 * @brief Returns normalized version of this vector without modifying it
		 * @return A new Vector2<T> made from this Vector2 with Magnitude == 1
		 */
		[[nodiscard]] constexpr GPM::Vector2<T> normalized() const;

		/**
		 * @brief Returns length of this Vector2
		 * @return Vector2 length
		 */
		[[nodiscard]] constexpr T Magnitude() const;

		/**
		 * @brief Calculates Dot Product between this Vector and another
		 * @param p_other Vector to calculate Dot Product with
		 * @return Dot Product
		 */
		template <typename U>
		constexpr T Dot(const Vector2<U>& p_other) const;

		/**
		 * @brief Calculate the distance between this vector and another one
		 * @param p_other The vector to compare distance with
		 * @return The distance
		 */
		template <typename U>
		constexpr T Distance(const Vector2<U>& p_other) const;

		/**
		 * @brief Returns a Vector2<T> that is perpendicular to this Vector2
		 */
		[[nodiscard]] constexpr Vector2<T> Perpendicular() const;

		/**
		 * @brief Multiplies this Vector2's x,y values by a given scalar
		 * @param p_scalar Scalar with which to multiply this Vector's x,y values
		 */
		constexpr void Scale(T p_scalar);

#pragma endregion
#pragma region Static

		/**
		 * @brief Calculates a normalized version of Vector given by parameter
		 * @param p_vector2 Vector from which normal is calculated
		 * @return Vector2<T> that is a normalized version of Vector given by parameter
		 */
		static constexpr Vector2<T> normalized(const Vector2<T>& p_vector2);

		/**
		 * @brief Normalizes Vector given as parameter (Magnitude will now equal 1)
		 * @param p_vector2 Vector to modify
		 */
		static constexpr void Normalize(Vector2<T>& p_vector2);

		/**
		 * @brief Calculates Dot Product between 2 given Vectors
		 * @param p_vector2Left First Vector
		 * @param p_vector2Right Second Vector
		 * @return Result of Dot Product
		 */
		template <typename U>
		static constexpr T Dot(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right);

		/**
		 * @brief Calculates Angle between 2 Vectors
		 * @param p_vector2Left First Vector
		 * @param p_vector2Right Second Vector
		 * @return Angle in radians
		 */
		template <typename U>
		static constexpr T Angle(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right);

		/**
		 * @Brief Calculates Distance between 2 given Vectors
		 * @param p_vector2Left First Vector
		 * @param p_vector2Right Second Vector
		 * @return Distance between 2 Vectors
		 */
		template <typename U>
		static constexpr T Distance(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right);

		/**
		* @brief Returns the start Vector moving to the end Vector at step alpha
		* @param p_vector2Start The beginning vector
		* @param p_vector2End The ending vector
		* @param p_alpha Between 0 and 1, 0 is start, 1 is end
		* @return The result vector
		*/
		template <typename U>
		static constexpr Vector2<T> Lerp(const Vector2<T>& p_vector2Start, const Vector2<U>& p_vector2End, const float p_alpha);

		/**
		 * @brief Returns a Vector2<T> that is perpendicular to Vector2 given by parameter
		 * @return Perpendicular Vector2<T>
		 */
		template <typename U>
		static constexpr Vector2<T> Perpendicular(const Vector2<U>& p_vector2);

#pragma endregion

#pragma endregion

#pragma region Arithmetic Operations

#pragma region Non-Static

		/**
		 * @brief Add Vector given by parameter to this Vector
		 * @param p_otherVector2 Vector to add
		 */
		constexpr void Add(const GPM::Vector2<T>& p_otherVector2);

		/**
		* @brief Add Vector given by parameter to this Vector
		* @param p_otherVector2 Vector to add
		*/
		constexpr void Subtract(const GPM::Vector2<T>& p_otherVector2);

		/**
		 * @brief Divide this Vector's values by given scalar
		 * @param p_scalar Scalar by which to divide this Vector's values
		 */
		constexpr void Divide(const T& p_scalar);
		/**
		* @brief Multiply this Vector's values by given scalar
		* @param p_scalar Scalar by which to multiply this Vector's values
		*/

		constexpr void Multiply(const T& p_scalar);

		/**
		 * @brief Check if this Vector has equal values to Vector given by parameter
		 * @param p_otherVector2 Vector to compare this with
		 */
		template<typename U>
		constexpr bool Equals(const GPM::Vector2<U>& p_otherVector2) const;

#pragma endregion

#pragma region Static


		/**
		 * @brief Adds two Vectors without modifying their values
		 * @param p_vector2Left First Vector
		 * @param p_vector2Right Second Vector
		 * @return Vector2<T> Result of added 2 Vectors
		 */

		template <typename U>
		static constexpr GPM::Vector2<T> Add(const GPM::Vector2<T>& p_vector2Left, const GPM::Vector2<U>& p_vector2Right);

		/**
		 * @brief Add scalar to x and y of Vector given by parameter without modifying its values
		 * @param p_vector2 Vector to add the scalar to
		 * @param p_scalar The scalar
		 * @return Result of scalar addition to Vector
		 */
		static constexpr GPM::Vector2<T> Add(const GPM::Vector2<T>& p_vector2, const T& p_scalar);

		/**
		* @brief Subtracts two Vectors without modifying their values
		* @param p_vector2Left First Vector
		* @param p_vector2Right Second Vector
		* @return Vector2<T> Result of subtracted 2 Vectors
		*/

		template <typename U>
		static constexpr GPM::Vector2<T> Subtract(const GPM::Vector2<T>& p_vector2Left, const GPM::Vector2<U>& p_vector2Right);

		/**
		* @brief Subtract scalar to x and y of Vector given by parameter without modifying its values
		* @param p_vector2 Vector to subtract the scalar to
		* @param p_scalar The scalar
		* @return Result of scalar subtraction to Vector
		*/
		static constexpr GPM::Vector2<T> Subtract(const GPM::Vector2<T>& p_vector2, const T& p_scalar);

		/**
		* @brief Multiply given Vector by a scalar without modifying it
		* @param p_vector2 Vector to multiply the scalar with
		* @param p_scalar The scalar
		* @return Result of scalar multiplication to Vector
		*/

		static constexpr GPM::Vector2<T> Multiply(const GPM::Vector2<T>& p_vector2, const T& p_scalar);
		/**
		* @brief Divide given Vector by a scalar without modifying it
		* @param p_vector2 Vector to divide the scalar with
		* @param p_scalar The scalar
		* @return Result of scalar division to Vector
		*/
		static constexpr GPM::Vector2<T> Divide(const GPM::Vector2<T>& p_vector2, const T& p_scalar);

		template <typename U>
		constexpr operator struct Vector2<U>() const { return Vector2<U>{ static_cast<U>(x), static_cast<U>(y) }; }

		/**
		* @brief Return the value aliased with index, just like arrays
		* @param p_index The index to access. 0 = x, 1 = y
		* @return Return the value associated at the indicated index
		* @note Vector4 representation is as follow : [x, y]
		*/
		constexpr T operator[](const int p_index) const;
#pragma endregion
#pragma endregion
	};

#pragma region Non-member Operator Overloads

	template <typename T>
	constexpr std::ostream& operator<<(std::ostream& p_stream, const Vector2<T>& p_vector);

	template<typename T, typename U>
	constexpr Vector2<T> operator+(Vector2<T> const& p_vector2Left, Vector2<U> const& p_vector2Right);

	template<typename T>
	constexpr Vector2<T> operator+(Vector2<T> const& p_vector2Left, Vector2<T> const& p_vector2Right);

	template<typename T, typename U>
	constexpr Vector2<T> operator+(Vector2<T> const& p_vector2, U const& p_scalar);

	template<typename T, typename U>
	constexpr Vector2<T> operator+(U const& p_scalar, Vector2<T> const& p_vector2);

	template<typename T>
	constexpr Vector2<T> operator-(Vector2<T> const& p_vector2Left, Vector2<T> const& p_vector2Right);

	template<typename T, typename U>
	constexpr Vector2<T> operator-(Vector2<T> const& p_vector2Left, Vector2<U> const& p_vector2Right);

	template<typename T, typename U>
	constexpr Vector2<T> operator-(Vector2<T> const& p_vector2, U const& p_scalar);

	template<typename T, typename U>
	constexpr Vector2<T> operator-(U const& p_scalar, Vector2<T> const& p_vector2);

	template<typename T, typename U>
	constexpr Vector2<U> operator*(const T& p_scalar, const Vector2<U>& p_vector2);

	template<typename T, typename U>
	constexpr Vector2<T> operator*(const Vector2<T>& p_vector2, const U& p_scalar);

	template<typename T, typename U>
	constexpr Vector2<T> operator*(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right);

	template<typename T, typename U>
	constexpr Vector2<T> operator/(Vector2<T> const& p_vector2, const U& p_scalar);

	template <typename T, typename U>
	constexpr void operator+=(Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right);

	template <typename T, typename U>
	constexpr void operator-=(Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right);

	template <typename T, typename U>
	constexpr void operator*=(Vector2<T>& p_vector2Left, const U& p_scalar);

	template <typename T, typename U>
	constexpr void operator/=(Vector2<T>& p_vector2Left, const U& p_scalar);
#pragma endregion

	using Vector2U = GPM::Vector2<unsigned int>;
	using Vector2I = GPM::Vector2<int>;
	using Vector2F = GPM::Vector2<float>;
	using Vector2D = GPM::Vector2<double>;
	using Vector2L = GPM::Vector2<long>;
}

#include <GPM/Vector/Vector2.inl>