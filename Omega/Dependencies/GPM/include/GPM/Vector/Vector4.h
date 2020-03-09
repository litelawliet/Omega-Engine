#pragma once

#include <type_traits>
#include <string>
#include <ostream>
#include <GPM/Vector/Vector3.h>

namespace GPM
{
	template<typename T>
	struct Vector4 final
	{
		static_assert(std::is_arithmetic<T>::value, "Vector4 should only be used with arithmetic types");

		T x;
		T y;
		T z;
		T w;

		static const Vector4<T> zero;
		static const Vector4<T> one;
		static const Vector4<T> xAxis;
		static const Vector4<T> yAxis;
		static const Vector4<T> zAxis;

#pragma region Constructors & Assignment
		/**
		 * @brief Default Constructor
		 */
		constexpr Vector4();

		/**
		 * @brief Destructor
		 */
		~Vector4() = default;

		/**
		 * @brief Constructor with parameters
		 * @param p_x x coordinate
		 * @param p_y y coordinate
		 * @param p_z z coordinate
		 * @param p_w Set to 1 for vectors, 0 for points
		 */
		constexpr Vector4(const T p_x, const T p_y, const T p_z, const T p_w = 1.0f);

		/**
		* @brief Constructor from vector3
		* @param p_other The vector to construct from
		*/
		explicit constexpr Vector4(const Vector3<T>& p_other);

		/**
		* @brief Move Constructor from vector3
		* @param p_other The vector to construct from
		*/
		explicit constexpr Vector4(Vector3<T>&& p_other);

		/**
		 * @brief Copy Constructor
		 * @param p_other The vector to construct from
		 */
		constexpr Vector4(const Vector4<T>& p_other);

		/**
		 * @brief Move Constructor
		 * @param p_other The vector to construct from
		 */
		constexpr Vector4(Vector4<T>&& p_other) noexcept;

		/**
		 * @brief Overload = operator by copy
		 * @param p_other The vector to construct from
		 * @return The current vector modified
		 */
		constexpr Vector4<T>& operator=(const Vector4<T>& p_other);

		/**
		 * @brief Overload = operator by move
		 * @param p_other The vector to construct from
		 * @return The current vector modified
		 */
		constexpr Vector4<T>& operator=(Vector4<T>&& p_other) noexcept;
#pragma endregion
#pragma region Tests & Comparisons

		/**
		 * @brief Return true if the two vectors are parallel
		 * @param p_other The vector used for the checkup
		 * @return True or false
		 */
		constexpr bool IsParallelTo(const Vector4<T>& p_other) const;

		/**
		 * @brief Return true if the two vectors are parallel
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return True or false
		 */
		constexpr static bool AreParallel(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Return true if the two vectors are perpendicular
		 * @param p_other The vector used for the checkup
		 * @return True or false
		 */
		constexpr bool IsPerpendicularTo(const Vector4<T>& p_other) const;

		/**
		 * @brief Return true if the two vectors are perpendicular
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return True or false
		 */
		constexpr static bool ArePerpendicular(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Return true if the vector is homogenized
		 * @return True or false
		 */
		constexpr bool IsHomogenized() const;

		/**
		 * @brief Return true if the vector is homogenized
		 * @param p_vector The vector used for the checkup
		 * @return True or false
		 */
		constexpr static bool IsHomogenized(const Vector4<T>& p_vector);

		/**
		 * @brief Return true if the two vectors are identical
		 * @param p_other The vector used for the checkup
		 * @return True or false
		 */
		constexpr bool IsEqualTo(const Vector4<T>& p_other) const;

		/**
		 * @brief Return true if the two vectors are identical
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return True or false
		 */
		constexpr static bool AreEqual(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Return true if the two vectors are identical
		 * @param p_other The vector used for the checkup
		 * @return True or false
		 */
		constexpr bool operator==(const Vector4<T>& p_other) const;

#pragma endregion
#pragma region Arithmetic Operations

#pragma region Add

		/**
		 * @brief Add scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		Vector4<T>& Add(const T p_scalar);

		/**
		 * @brief Add scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		template<typename U>
		Vector4<T>& Add(const U p_scalar);

		/**
		 * @brief Add scalar to vector left
		 * @param p_left The left vector to add
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		constexpr static Vector4<T> Add(const Vector4<T>& p_left, const T p_scalar);

		/**
		 * @brief Add scalar to vector left
		 * @param p_left The left vector to add
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		template<typename U>
		constexpr static Vector4<T> Add(const Vector4<T>& p_left, const U p_scalar);

		/**
		 * @brief Add other vector to the actual vector
		 * @param p_other The vector to add
		 * @return The current vector modified
		 */
		Vector4<T>& Add(const Vector4<T>& p_other);

		/**
		 * @brief Add other vector to the actual vector
		 * @param p_other The vector to add
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& Add(const Vector4<U>& p_other);

		/**
		 * @brief Add left vector to the right vector
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return The result vector
		 */
		constexpr static Vector4<T> Add(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Add left vector to the right vector
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return The result vector
		 */
		template <typename U>
		constexpr static Vector4<T> Add(const Vector4<T>& p_left, const Vector4<U>& p_right);

		/**
		* @brief Return the summation of other vector and actual vector
		* @param p_scalar The scalar
		* @return The result vector
		*/
		constexpr Vector4<T> operator+(const T p_scalar) const;

		/**
		* @brief Add other vector to the actual vector
		* @param p_scalar The scalar
		* @return The current vector modified
		*/
		Vector4<T>& operator+=(const T p_scalar);

		/**
		 * @brief Return the summation of other vector and actual vector
		 * @param p_other The other vector
		 * @return The result vector
		 */
		constexpr Vector4<T> operator+(const Vector4<T>& p_other) const;

		/**
		 * @brief Return the summation of other vector and actual vector
		 * @param p_other The other vector
		 * @return The result vector
		 */
		template <typename U>
		constexpr Vector4<T> operator+(const Vector4<U>& p_other) const;

		/**
		 * @brief Add other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		Vector4<T>& operator+=(const Vector4<T>& p_other);

		/**
		 * @brief Add other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& operator+=(const Vector4<U>& p_other);

#pragma endregion
#pragma region Substract

		/**
		 * @brief Subtract scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		Vector4<T>& Subtract(const T p_scalar);

		/**
		 * @brief Subtract scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& Subtract(const U p_scalar);

		/**
		 * @brief Subtract scalar to vector left
		 * @param p_left The left vector
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		constexpr static Vector4<T> Subtract(const Vector4<T>& p_left, const T p_scalar);

		/**
		 * @brief Subtract scalar to vector left
		 * @param p_left The left vector
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		template <typename U>
		constexpr static Vector4<T> Subtract(const Vector4<T>& p_left, const U p_scalar);

		/**
		 * @brief Subtract other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		Vector4<T>& Subtract(const Vector4<T>& p_other);

		/**
		 * @brief Subtract other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& Subtract(const Vector4<U>& p_other);

		/**
		 * @brief Subtract left vector to the right vector
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return The result vector
		 */
		constexpr static Vector4<T> Subtract(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Subtract left vector to the right vector
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return The result vector
		 */
		template<typename U>
		constexpr static Vector4<T> Subtract(const Vector4<T>& p_left, const Vector4<U>& p_right);

		/**
		* @brief Return the subtraction of other vector and actual vector
		* @param p_scalar The scalar
		* @return The result vector
		*/
		constexpr Vector4<T> operator-(const T p_scalar) const;

		/**
		* @brief Subtract other vector to the actual vector
		* @param p_scalar The scalar
		* @return The current vector modified
		*/
		Vector4<T>& operator-=(const T p_scalar);

		/**
		 * @brief Return the subtraction of other vector and actual vector
		 * @param p_other The other vector
		 * @return The result vector
		 */
		constexpr Vector4<T> operator-(const Vector4<T>& p_other) const;

		/**
		 * @brief Return the subtraction of other vector and actual vector
		 * @param p_other The other vector
		 * @return The result vector
		 */
		template <typename U>
		constexpr Vector4<T> operator-(const Vector4<U>& p_other) const;

		/**
		 * @brief Subtract other vector to the actual vector
		 * @param p_other The vector to use
		 * @return The current vector modified
		 */
		Vector4<T>& operator-=(const Vector4<T>& p_other);

		/**
		 * @brief Subtract other vector to the actual vector
		 * @param p_other The vector to use
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& operator-=(const Vector4<U>& p_other);

#pragma endregion
#pragma region Multiply

		/**
		 * @brief Multiply scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		Vector4<T>& Multiply(const T p_scalar);

		/**
		 * @brief Multiply scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& Multiply(const U p_scalar);

		/**
		 * @brief Multiply scalar to vector left
		 * @param p_left The vector to multiply
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		constexpr static Vector4<T> Multiply(const Vector4<T>& p_left, T p_scalar);

		/**
		 * @brief Multiply scalar to vector left
		 * @param p_left The vector to multiply
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		template <typename U>
		constexpr static Vector4<T> Multiply(const Vector4<T>& p_left, U p_scalar);

		/**
		 * @brief Multiply other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		Vector4<T>& Multiply(const Vector4<T>& p_other);

		/**
		 * @brief Multiply other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& Multiply(const Vector4<U>& p_other);

		/**
		 * @brief Return the multiplication of scalar and actual vector
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		constexpr Vector4<T> operator*(const T p_scalar) const;

		/**
		 * @brief Multiply scalar to the actual vector
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		Vector4<T>& operator*=(const T p_scalar);

		/**
		 * @brief Return the multiplication of a vector and actual vector
		 * @param p_other The scalar
		 * @return The result vector
		 */
		constexpr Vector4<T> operator*(const Vector4<T>& p_other) const;

		/**
		 * @brief Return the multiplication of a vector and actual vector
		 * @param p_other The vector
		 * @return The result vector
		 */
		template <typename U>
		constexpr Vector4<T> operator*(const Vector4<U>& p_other) const;

		/**
		 * @brief Multiply vector to the actual vector
		 * @param p_other The vector
		 * @return The current vector modified
		 */
		Vector4<T>& operator*=(const Vector4<T>& p_other);

		/**
		 * @brief Multiply vector to the actual vector
		 * @param p_other The vector
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& operator*=(const Vector4<U>& p_other);

#pragma endregion
#pragma region Divide

		/**
		 * @brief Divide scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		Vector4<T>& Divide(const T p_scalar);

		/**
		 * @brief Divide scalar to x, y and z
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& Divide(const U p_scalar);

		/**
		 * @brief Divide scalar to vector left
		 * @param p_left The vector to divide
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		constexpr static Vector4<T> Divide(const Vector4<T>& p_left, const T p_scalar);

		/**
		 * @brief Divide scalar to vector left
		 * @param p_left The vector to divide
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		template <typename U>
		constexpr static Vector4<T> Divide(const Vector4<T>& p_left, const U p_scalar);

		/**
		 * @brief Divide other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		Vector4<T>& Divide(const Vector4<T>& p_other);

		/**
		 * @brief Divide other vector to the actual vector
		 * @param p_other The other vector
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& Divide(const Vector4<U>& p_other);
		
		/**
		 * @brief Return the division of scalar and actual vector
		 * @param p_scalar The scalar
		 * @return The result vector
		 */
		constexpr Vector4<T> operator/(const T p_scalar) const;

		/**
		 * @brief Divide scalar to the actual vector
		 * @param p_scalar The scalar
		 * @return The current vector modified
		 */
		Vector4<T>& operator/=(const T p_scalar);

		/**
		 * @brief Return the division of a vector and actual vector
		 * @param p_other The scalar
		 * @return The result vector
		 */
		constexpr Vector4<T> operator/(const Vector4<T>& p_other) const;

		/**
		 * @brief Return the division of a vector and actual vector
		 * @param p_other The vector
		 * @return The result vector
		 */
		template <typename U>
		constexpr Vector4<T> operator/(const Vector4<U>& p_other) const;

		/**
		 * @brief Divide vector to the actual vector
		 * @param p_other The vector
		 * @return The current vector modified
		 */
		Vector4<T>& operator/=(const Vector4<T>& p_other);

		/**
		 * @brief Divide vector to the actual vector
		 * @param p_other The vector
		 * @return The current vector modified
		 */
		template <typename U>
		Vector4<T>& operator/=(const Vector4<U>& p_other);

#pragma endregion

#pragma endregion

#pragma region Vector Operations

		/**
		 * @brief Calculate the distance between the vector and another
		 * @param p_vector The vector to compare distance
		 * @return The distance
		 */
		constexpr float Distance(const Vector4<T>& p_vector) const;

		/**
		 * @brief Calculate the distance between the vector and another
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return The distance
		 */
		constexpr static float Distance(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Scale the vector with scalar
		 * @param p_scale The scalar
		 * @return The current vector modified
		 */
		constexpr Vector4<T>& Scale(const T p_scale);

		/**
		 * @brief Calculate the distance between the vector and another
		 * @param p_vector The vector to scale
		 * @param p_scale The scalar
		 * @return The result vector
		 */
		constexpr static Vector4<T> Scale(const Vector4<T>& p_vector, const T p_scale);

		/**
		 * @brief Calculate the length of the vector
		 * @return The length of the vector
		 */
		constexpr T Magnitude() const;

		/**
		 * @brief Calculate the length of the vector
		 * @param p_vector The vector on which we calculate the magnitude
		 * @return The length of the vector
		 */
		constexpr static T Magnitude(const Vector4<T>& p_vector);

		/**
		 * @brief Calculate the squared length of the vector
		 * @return The length square of the vector
		 */
		constexpr T MagnitudeSquare() const;

		/**
		 * @brief Calculate the squared length of the vector
		 * @param p_vector The vector on which we calculate the square magnitude
		 * @return The length square of the vector
		 */
		constexpr static T MagnitudeSquare(const Vector4<T>& p_vector);

		/**
		 * @brief Calculate the dot product with other vector
		 * @param p_other The other vector for dot product
		 * @return The scale between two vectors
		 */
		constexpr T Dot(const Vector4<T>& p_other) const;

		/**
		 * @brief Calculate the dot product between two vectors
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return The scale between two vectors
		 */
		constexpr static T Dot(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Calculate the cross product with other vector
		 * @param p_other The other vector for cross product
		 * @return The result vector
		 */
		constexpr Vector4<T> Cross(const Vector4<T>& p_other) const;

		/**
		 * @brief Calculate the cross product between two vectors
		 * @param p_left Left vector
		 * @param p_right Right vector
		 * @return The result vector
		 */
		constexpr static Vector4<T> Cross(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Calculate the dot product between left and the result of cross product between middle and right
		 * @param p_left Left product
		 * @param p_middle Middle vector
		 * @param p_right Right vector
		 * @return The result of the triple product
		 */
		constexpr static T TripleProduct(const Vector4<T>& p_left, const Vector4<T>& p_middle, const Vector4<T>& p_right);

		/**
		 * @brief Calculate the angle between two vectors in radiant
		 * @param p_other The vector to lookup for angle
		 * @return The angle between the two vectors
		 */
		constexpr T Angle(const Vector4<T>& p_other) const;

		/**
		 * @brief Calculate the angle between two vectors in radiant
		 * @param p_left The left vector
		 * @param p_right The right vector
		 * @return The angle between the two vectors
		 */
		constexpr static T Angle(const Vector4<T>& p_left, const Vector4<T>& p_right);

		/**
		 * @brief Normalize the vector
		 * @return The current vector modified
		 */
		Vector4<T>& Normalize();

		/**
		 * @brief Return the normalized vector
		 * @param p_vector The vector to normalized
		 * @return The result vector
		 */
		constexpr static Vector4<T> Normalize(const Vector4<T>& p_vector);

		/**
		* @brief Homogenize the vector
		* @return The current vector homogenized
		*/
		Vector4<T>& Homogenize();

		/**
		 * @brief Return the homogenized vector
		 * @param p_vector The vector to homogenize
		 * @return The result vector
		 */
		constexpr static Vector4<T> Homogenize(const Vector4<T>& p_vector);

		/**
		* @brief Return the start vector moving to the end vector at step interpolationCoefficient
		* @param p_start The beginning vector
		* @param p_end The ending vector
		* @param p_interpolationCoefficient Between 0 and 1, 0 is start, 1 is end
		* @return The result vector
		*/
		constexpr static Vector4<T> Lerp(const Vector4<T>& p_start, const Vector4<T>& p_end, const float p_interpolationCoefficient);

		/**
		* @brief Return the start vector moving to the end vector at step interpolationCoefficient
		* @param p_start The beginning vector
		* @param p_end The ending vector
		* @param p_interpolationCoefficient Between 0 and 1, 0 is start, 1 is end
		* @return The result vector
		*/
		constexpr static Vector4<T> Slerp(const Vector4<T>& p_start, const Vector4<T>& p_end, const float p_interpolationCoefficient);

		/**
		* @brief Return the start vector moving to the end vector at step interpolationCoefficient
		* @param p_start The beginning vector
		* @param p_end The ending vector
		* @param p_interpolationCoefficient Between 0 and 1, 0 is start, 1 is end
		* @return The result vector
		*/
		constexpr static Vector4<T> Nlerp(const Vector4<T>& p_start, const Vector4<T>& p_end, const float p_interpolationCoefficient);

		/**
		 * @brief Return the value aliased with index, just like arrays
		 * @param p_index The index to access. 0 = x, 1 = y, 2 = z, 3 = w
		 * @return Return the value associated at the indicated index
		 * @note Vector4 representation is as follow : [x, y, z, w]
		 */
		constexpr T operator[](const int p_index) const;
		
#pragma endregion
#pragma region Conversions

		/**
		* @brief Convert vector to string
		* @return The output string
		*/
		constexpr std::string ToString() const;

		/**
		* @brief Convert vector to string
		* @param p_vector The vector to print
		* @return The output string
		*/
		constexpr static std::string ToString(const Vector4<T>& p_vector);

#pragma endregion
	};

#pragma region Outside Operators

	template <typename T>
	constexpr std::ostream& operator<<(std::ostream& p_stream, const Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T> operator+(const T p_scalar, const Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T> operator+(const U p_scalar, const Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T>& operator+=(const T p_scalar, Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T>& operator+=(const U p_scalar, Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T> operator-(const T p_scalar, const Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T> operator-(const U p_scalar, const Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T>& operator-=(const T p_scalar, Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T>& operator-=(const U p_scalar, Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T> operator*(const T p_scalar, const Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T> operator*(const U p_scalar, const Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T>& operator*=(const T p_scalar, Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T>& operator*=(const U p_scalar, Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T> operator/(const T p_scalar, const Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T> operator/(const U p_scalar, const Vector4<T>& p_vector);

	template <typename T>
	constexpr Vector4<T>& operator/=(const T p_scalar, Vector4<T>& p_vector);

	template <typename T, typename U>
	constexpr Vector4<T>& operator/=(const U p_scalar, Vector4<T>& p_vector);

#pragma endregion

	using Vector4F = Vector4<float>;
	using Vector4D = Vector4<double>;
	using Vector4I = Vector4<int>;
	using Vector4L = Vector4<long>;
}

#include <GPM/Vector/Vector4.inl>