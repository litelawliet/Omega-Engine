#pragma once
#include <string>

namespace GPM
{
	struct Quaternion final
	{

		//m_w is the real value of quaternion, this will be used to check if the quaternion is pure/identity or not.
		double w;
		double x;
		double y;
		double z;

		// static const Quaternion identity;

#pragma region Constructors & Assignment
		inline Quaternion();
		/**
		 * @brief Constructor using all values
		 * @param p_x Vector part of Quaternion
		 * @param p_y Vector part of Quaternion
		 * @param p_z Vector part of Quaternion
		 * @param p_w Real value of Quaternion
		 *
		 * @note In pure/applied Maths, we write W (or real), (Xi + Yj + Zk) (or Vector)
		 */
		inline Quaternion(const double p_x, const double p_y, const double p_z, const double p_w);

		/**
		 * @brief Constructor using a scalar and a vector
		 * @param p_scalar The scalar
		 * @param p_vector The vector
		 */
		inline Quaternion(const double p_scalar, const Vector3<double>& p_vector);

		/**
		 * @brief Copy Constructor
		 * @param p_other
		 */
		inline Quaternion(const Quaternion& p_other);

		/**
		 * @brief Move Constructor
		 * @param p_other
		 */
		inline Quaternion(Quaternion&& p_other) noexcept;

		/**
		 * @brief Construct from rotation matrix
		 * @param p_matrix Rotation matrix
		 */
		inline Quaternion(const Matrix3<double>& p_matrix);

		/**
		 * @brief Construct from rotation matrix
		 * @param p_matrix Rotation matrix
		 */
		inline Quaternion(const Matrix4<double>& p_matrix);

		/**
		 * @brief Construct from rotation matrix
		 * @param p_matrix Rotation matrix
		 */
		inline Quaternion(const Matrix4<float>& p_matrix);

		/**
		 * @brief Constuct a quaternion from axis and angle in radian
		 * @param p_axis
		 * @param p_angleInRadians
		 */
		inline Quaternion(const Vector3<double>& p_axis, const double p_angleInRadians);

		~Quaternion() = default;

		/**
		 * @brief Construct from euler angles
		 * @param p_yawAlpha The x-angle in degree
		 * @param p_roll The y-angle in degree
		 * @param p_yaw The z-angle in degree
		 * @return The quaternion made from euler angles
		 */
		[[nodiscard]] static inline Quaternion MakeFromEuler(const double p_yawAlpha, const double p_roll, const double p_yaw);

		/**
		 * @brief Construct from euler angles
		 * @param p_euler A vector representing the euler angle in degree
		 * @return The quaternion made from euler angles
		 */
		[[nodiscard]] static inline Quaternion MakeFromEuler(const Vector3<double>& p_euler);

		/**
		 * @brief Set a quaternion from euler angles
		 * @param p_euler A vector representing the euler angle in degree
		 */
		inline void SetFromEuler(const Vector3<double>& p_euler) const;

		/**
		 * @brief Set a quaternion from euler angles
		 * @param p_roll The x-angle in degree
		 * @param p_pith The y-angle in degree
		 * @param p_yaw The z-angle in degree
		 */
		inline void SetFromEuler(const double p_roll, const double p_pith, const double p_yaw);

		/**
		 * @brief Copy assignment
		 * @param p_other The quaternion
		 * @return The current quaternion modified
		 */
		inline Quaternion& operator=(const Quaternion& p_other) = default;

		/**
		 * @brief Move assigment
		 * @param p_other The quaternion
		 * @return The current quaternion modified
		 */
		inline Quaternion& operator=(Quaternion&& p_other) noexcept;
#pragma endregion

#pragma region Tests & Comparisons

		/**
		 * @brief Check If the quaternion is Identity
		 * @note If the quaternion has no rotation(meaning x,y,z axis values = 0), it's Identity
		 * @return True or false
		 */
		[[nodiscard]] bool IsIdentity() const;

		/**
		 * @brief Check if the quaternion is pure
		 * @note If the quaternion has no real value(meaning real part = 0), it's pure
		 * @return True or false
		 */
		[[nodiscard]] bool IsPure() const;

		/**
		 * @brief Check if the quaternion is normalized
		 * @note A quaternion is normalized if his magnitude is equal to 1
		 * @return True or false
		 */
		[[nodiscard]] bool IsNormalized() const;

		/**
		 * @brief Check is all components between the current quaternion and the other one are equals
		 * @param p_otherQuaternion The other quaternion to check with
		 */
		bool operator==(const Quaternion& p_otherQuaternion) const;

		/**
		 * @brief Check is all components between the current quaternion and the other one are different
		 * @param p_otherQuaternion The other quaternion to check with
		 */
		bool operator!=(const Quaternion& p_otherQuaternion) const;

#pragma endregion
#pragma region Arithmetic Operations

#pragma region Add
		Quaternion operator+(const Quaternion& p_otherQuaternion) const;
		Quaternion& operator+=(const Quaternion& p_otherQuaternion);

#pragma endregion
#pragma region Substract
		Quaternion operator-(const Quaternion& p_otherQuaternion) const;
		Quaternion& operator-=(const Quaternion& p_otherQuaternion);

#pragma endregion
#pragma region Multiply
		/**
		 * @brief Return the dot product between the current quaternion and another one
		 * @param p_otherQuaternion The other quaternion
		 * @return The result
		 */
		[[nodiscard]] double DotProduct(const Quaternion& p_otherQuaternion) const;

		/**
		 * @brief Return the dot product between the current quaternion and another one
		 * @param p_left The left quaternion
		 * @param p_right The right quaternion
		 * @return The result
		 */
		static double DotProduct(const Quaternion& p_left, const Quaternion& p_right);

		inline Quaternion operator*(const double p_scale) const;
		inline Quaternion& operator*=(const double p_scale);

		inline Quaternion operator*(const Quaternion& p_otherQuaternion) const;
		inline Quaternion& operator*=(const Quaternion& p_otherQuaternion);

		inline Quaternion operator*(const Vector3<double>& p_toMultiply) const;
		inline Quaternion& operator*=(const Vector3<double>& p_toMultiply);

		inline Vector3<float> operator*(const Vector3<float>& p_toMultiply) const;


#pragma endregion
#pragma endregion

#pragma region Quaternion Operations
		/**
		 * @brief Normalize the current quaternion
		 * @return The current quaternion modified
		 */
		Quaternion& Normalize();

		/**
		 * @brief Normalize the current quaternion
		 * @param p_quaternion The quaternion to normalize
		 * @return A normalized quaternion
		 */
		static Quaternion Normalize(const Quaternion& p_quaternion);

		/**
		 * @brief Multiply the current quaternion with another one
		 * @param p_quaternion The quaternion to multiply
		 * @return The result of the multiplication between the two quaternion
		 */
		[[nodiscard]] inline Quaternion Multiply(const Quaternion& p_quaternion) const;

		/**
		 * @brief Norm of a quaternion, alias magnitude
		 * @return The magnitude
		 */
		[[nodiscard]] inline double Norm() const;

		/**
		 * @brief Norm square of a quaternion, alias magnitude square
		 * @return The magnitude squared
		 */
		[[nodiscard]] constexpr inline double NormSquare() const;

		/**
		 * @brief Actual angle of this quaternion
		 * @return The angle of the quaternion
		 */
		[[nodiscard]] inline double GetAngle() const;

		/**
		 * @brief Actual angle of this quaternion
		 * @return The angle of the quaternion
		 */
		[[nodiscard]] static inline double GetAngle(const Quaternion& p_target);

		/**
		 * @brief Inverse the current quaternion
		 * @return The current quaternion modified
		 */
		Quaternion& Inverse();

		/**
		 * @brief Inverse the quaternion in parameter
		 * @param p_quaternion The quaternion
		 * @return A new quaternion inversed
		 */
		static Quaternion Inverse(const Quaternion& p_quaternion);

		/**
		 * @brief Conjugate the current quaternion
		 * @return The current quaternion modified
		 */
		Quaternion& Conjugate();

		/**
		 * @brief Conjugate the quaternion in parameter
		 * @param p_quaternion The quaternion
		 * @return A new quaternion conjugate
		 */
		static Quaternion Conjugate(const Quaternion& p_quaternion);

		/**
		 * @brief Convert the current quaternion to unit quaternion
		 * @return The current quaternion modified
		 */
		Quaternion& ConvertToUnitNormQuaternion();

		/**
		 * @brief Give the axis of the quaternion
		 * @return An axis
		 */
		[[nodiscard]] Vector3<double> GetRotationAxis() const;

		//double AngularDistance(const Quaternion& p_other) const;

		/**
		 * @brief Return the x value of the axis
		 * @return The value
		 */
		[[nodiscard]] double GetXAxisValue() const;

		/**
		 * @brief Return the y value of the axis
		 * @return The value
		 */
		[[nodiscard]] double GetYAxisValue() const;

		/**
		 * @brief Return the z value of the axis
		 * @return The value
		 */
		[[nodiscard]] double GetZAxisValue() const;

		/**
		 * @brief Return the w component (real part)
		 * @return The value
		 */
		[[nodiscard]] double GetRealValue() const;


		/**
		 * @brief Set the x value of the axis
		 * @param p_xValue New value
		 */
		void SetXAxisValue(const double p_xValue);

		/**
		 * @brief Set the y value of the axis
		 * @param p_yValue New value
		 */
		void SetYAxisValue(const double p_yValue);

		/**
		 * @brief Set the z value of the axis
		 * @param p_zValue New value
		 */
		void SetZAxisValue(const double p_zValue);

		/**
		 * @brief Set the w component (real part)
		 * @param p_realValue New value
		 */
		void SetRealValue(const double p_realValue);

		/**
		 * @brief Creates a rotation with the specified forward and upwards directions.
		 * @param p_forward Forward direction
		 * @param p_upwards Upwards direction
		 * @return The quaternion
		 */
		[[nodiscard]] Quaternion LookRotation(const Vector3<double>& p_forward, const Vector3<double>& p_upwards = Vector3<double>::up) const;

		/**
		 * @brief Create a quaternion out of an axis and angle
		 * @param p_axis The axis
		 * @param p_angle The angle
		 * @return The quaternion
		 */
		static Quaternion CreateFromAxisAngle(const Vector3<double>& p_axis, const double p_angle);

		/**
		 * @brief Interpolation between two quaternions
		 * @param p_start Start quaternion
		 * @param p_end End quaternion
		 * @param p_alpha Coefficient
		 * @return The quaternion
		 */
		static Quaternion Lerp(const Quaternion& p_start, const Quaternion& p_end, const double p_alpha);

		/**
		 * @brief Smoothly interpolate between two quaternions
		 * @param p_start Start quaternion
		 * @param p_end End quaternion
		 * @param p_alpha Coefficient
		 * @return The quaternion
		 */
		static Quaternion Slerp(const Quaternion& p_start, const Quaternion& p_end, const double p_alpha);

		/**
		 * @brief Smoothly interpolate between two quaternions and use the shortest path to it. Prevents wrong side rotation.
		 * @param p_start Start quaternion
		 * @param p_end End quaternion
		 * @param p_alpha Coefficient
		 * @return The quaternion
		 */
		static Quaternion SlerpShortestPath(const Quaternion& p_start, const Quaternion& p_end, double p_alpha);

		/**
		 * @brief Normalized interpolate between two quaternions
		 * @param p_start Start quaternion
		 * @param p_end End quaternion
		 * @param p_alpha Coefficient
		 * @return The quaternion
		 */
		static Quaternion Nlerp(const Quaternion& p_start, const Quaternion& p_end, const double p_alpha);

		/**
		 * @brief Rotate a point relative to pivot
		 * @param p_point The point to rotate around
		 * @param p_quaternion The rotation
		 * @return The new position
		 */
		[[nodiscard]] inline Vector3<double> RotateRelativeToPivot(const Vector3<double>& p_point, const Quaternion& p_quaternion) const;

		/**
		 * @brief Rotate a point relative to pivot using a quaternion
		 * @param p_point The point to rotate
		 * @param p_pivot The point of pivot
		 * @param p_quaternion The rotation
		 * @return The new position
		*/
		[[nodiscard]] static inline Vector3<double> RotateRelativeToPivot(const Vector3<double>& p_point, const Vector3<double>& p_pivot,
			const Quaternion& p_quaternion);

		/**
		 * @brief Rotate the vector of a certain angle around an arbitrary axis
		 * @param p_angle The angle
		 * @param p_axis The axis
		 * @param p_vectorToRotate Vector to rotate
		 * @return The vector rotated
		*/
		static Vector3<double> RotateVectorAboutAngleAndAxis(const double p_angle, const Vector3<double>& p_axis, const Vector3<double>& p_vectorToRotate);

		/**
		 * @brief Return the value aliased with index, just like arrays
		 * @param p_index The index to access. 0 = w, 1 = x, 2 = y, 3 = z
		 * @return Return the value associated at the indicated index
		 * @note Quaternion representation is as follow : [w, x, y, z]
		 */
		double operator[](const int p_index) const;

#pragma endregion
#pragma region Conversions
		/**
		 * @brief Transform the current quaternion to a unit quaternion
		 * @return The quaternion
		 */
		Quaternion ToUnitNormQuaternion();

		/**
		 * @brief Transform the current quaternion to euler angles in degrees
		 * @return A vector containing each angles
		 */
		[[nodiscard]] Vector3<float> ToEuler() const;

		/**
		 * @brief Create a quaternion from euler in degrees
		 * @param p_euler The euler angle in degree
		 * @return The quaternion
		 */
		static Quaternion ToQuaternion(const Vector3<double>& p_euler);

		/**
		 * @brief Create a quaternion from yaw, pitch and roll angle in degrees
		 * @param p_yaw The yaw angle
		 * @param p_pitch The pitch angle
		 * @param p_roll The roll angle
		 * @return The quaternion
		 */
		static Quaternion ToQuaternion(const double p_yaw, const double p_pitch, const double p_roll);

		/**
		 * @brief Transform the current quaternion to string
		 * @return The converted string
		 */
		[[nodiscard]] std::string ToString() const;

		/**
		 * @brief Transform a quaternion to string
		 * @param p_quaternion The quaternion to turn into string
		 * @return The converted string
		 */
		static std::string ToString(const Quaternion& p_quaternion);

		/**
		 * @brief Return a Matrix3 of double out of the quaternion
		 * @return The Matrix3<float>
		 */
		[[nodiscard]] Matrix3<float> ToMatrix3() const;

		/**
		 * @brief Return a Matrix4 of double out of the quaternion
		 * @return The Matrix4<float>
		 */
		[[nodiscard]] Matrix4<float> ToMatrix4() const;
#pragma endregion
	};

	std::ostream& operator<<(std::ostream& p_stream, const Quaternion& p_quaternion);
}

#include <GPM/Quaternion/Quaternion.inl>