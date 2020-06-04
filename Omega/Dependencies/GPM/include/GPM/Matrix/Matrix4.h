#pragma once
// Make your .inl here in include folder.
// #include <GPM/Quaternion/Quaternion.h>


namespace GPM
{
    struct Quaternion;
    /**
     * A standard 4 by 4 Matrix. Default value is an identity matrix
     */

    template<typename T>
    struct Matrix4
    {
        static_assert(std::is_arithmetic<T>::value, "Matrix4 should only be used with arithmetic types");

        //data
        T m_data[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };

        //constuctor
        constexpr Matrix4();
        ~Matrix4() = default;

        constexpr Matrix4(const Matrix4<T>& p_matrix);
        Matrix4(Matrix4<T>&& p_matrix) noexcept;

        constexpr Matrix4(const Vector3<T>& p_vector);

        constexpr Matrix4(const T p_data[16]);
        constexpr Matrix4(const T p_00, const T p_01, const T p_02, const T p_03,
                   const T p_10, const T p_11, const T p_12, const T p_13,
                   const T p_20, const T p_21, const T p_22, const T p_23,
                   const T p_30, const T p_31, const T p_32, const T p_33);

#pragma region Static Properties
        /**
         * @brief This is a const matrix that has 1 in the diagonal and 0 elsewhere
         */
        static Matrix4<T> identity;

        /**
         *  @brief This is a const matrix that has 0 everywhere
         */
        static Matrix4<T> zero;
#pragma endregion

#pragma region Properties

        constexpr bool isIdentity();

        constexpr T Determinant();
        constexpr static T Determinant(const Matrix4<T>& p_matrix);
        
        constexpr Matrix4<T>& Transpose();
        constexpr static Matrix4<T> Transpose(const Matrix4<T>& p_matrix);
        
        constexpr Matrix4<T>& Normalize();
        constexpr static Matrix4<T> Normalize(const Matrix4<T>& p_matrix);
        
        template<typename U>
        constexpr Matrix4<T>& Scale(const Vector3<U>& p_scale);
        template<typename U>
        constexpr static Matrix4<T> CreateScale(const Vector3<U>& p_scale);

        constexpr Matrix4<T>& Rotate(const Quaternion& p_rotation);
        constexpr static Matrix4<T> CreateRotation(const Quaternion& p_rotation);
        
        template<typename U>
        constexpr Matrix4<T>& Translate(const Vector3<U>& p_translate);
        template<typename U>
        constexpr static Matrix4<T> CreateTranslation(const Vector3<U>& p_translate);
        
        template<typename U>
        constexpr Matrix4<T>& Transform(const Vector3<U>& p_translate, const Quaternion& p_rotation, const Vector3<U>& p_scale);
        template<typename U>
        constexpr static Matrix4<T> CreateTransformation(const Vector3<U>& p_translate, const Quaternion& p_rotation, const Vector3<U>& p_scale);

        constexpr Matrix4<T> Adjugate();
        constexpr static Matrix4<T> CreateAdjugate(const Matrix4<T>& p_matrix);

        constexpr static Matrix4<T> Inverse(const Matrix4<T>& p_matrix);

    	/**
    	 *  @brief Rotate a matrix of a certain angle around a certain axis using matrices.
    	 *  @param p_matrix Matrix of reference
    	 *  @param p_angle Angle of the rotation in radian
    	 *  @param p_axis Axis of rotation
    	 *  @return A matrix rotated from the referenced one.
    	 */
        constexpr static Matrix4<T> Rotate(const Matrix4<T>& p_matrix, const T p_angle, const Vector3<T>& p_axis);

        /**
		 *  @brief View matrix made of the position, target and world's up vector.
		 *  @param p_eye  Position of the camera
		 *  @param p_target Point to look at
		 *  @param p_up Up axis of the world. (x=0, y=1, z=0) by default
		 *  @return The view matrix
		 */
        constexpr static Matrix4<T> LookAt(const Vector3<T>& p_eye, const Vector3<T>& p_target, const Vector3<T>& p_up = { 0, 1, 0 });

        /**
		 *  @brief Perspective projection matrix.
		 *  @param p_fovy Field of view
		 *  @param p_aspectRatio Aspect ratio of the window
		 *  @param p_near Minimal offset between the camera position and the rendering space
		 *  @param p_far Maximum offset between the camera position and the rendering space
		 *  @note Every objects that are outside p_near and p_far range is not rendered.
		 *  @return The perspective projection matrix
		 */
        constexpr static Matrix4<T> Perspective(const T p_fovy, const T p_aspectRatio, const T p_near, const T p_far);

        /**
		 *  @brief Orthographic projection matrix.
		 *  @param p_left Left corner of the rendering space
		 *  @param p_right Right corner of the rendering space
		 *  @param p_bottom Bottom corner of the rendering space
		 *  @param p_top Top corner of the rendering space
		 *  @note Depending on how the graphic API is rendering, you may have to reverse the top and bottom (and more rarely left and right as well).
		 *  @return The orthographic projection matrix
		 */
        constexpr static Matrix4<T> Orthographic(const T p_left, const T p_right, const T p_bottom, const T p_top, const T p_near = static_cast<T>(0), const T p_far = static_cast<T>(1000));
#pragma endregion

        constexpr void SetColumn(const int p_column, const Vector4<T>& p_vector);
        constexpr void SetRow(const int p_row, const Vector4<T>& p_vector);

        static T GetMinor(Matrix3<T> p_minor);


#pragma region Conversions

        [[nodiscard]] constexpr std::string ToString() const noexcept;
        constexpr static std::string ToString(const Matrix4<T>& p_matrix);

#pragma endregion

#pragma region Arithmetic Operations

#pragma region Add
        /**
         * @brief Add other matrix to the current matrix
         * @param p_other : The other matrix you want to add to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix4<T>& Add(const Matrix4<U>& p_other);

        /**
         * @brief Add left matrix to the right matrix
         * @param p_left : The first matrix you want to add
         * @param p_right : The second matrix to be added to the first one
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix4<T> Add(const Matrix4<T>& p_left, const Matrix4<U>& p_right);

        /**
         * @brief Return the summation of other matrix and current matrix
         * @param p_other : The other matrix you want to add
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr Matrix4<T> operator+(const Matrix4<U>& p_other) const;

        /**
         * @brief Add other matrix to the current matrix
         * @param p_other : The other matrix you want to add to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix4<T> operator+=(const Matrix4<U>& p_other);

#pragma endregion
#pragma region Subtract
        /**
         * @brief Subtract other matrix to the current matrix
         * @param p_other : The matrix you want to subtract to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix4<T> Subtract(const Matrix4<U>& p_other);

        /**
         * @brief Subtract left matrix to the right matrix
         * @param p_left : The first matrix you want to subtract to
         * @param p_right : The second matrix to be subtracted from the first one
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix4<T> Subtract(const Matrix4<T>& p_left, const Matrix4<U>& p_right);

        /**
         * @brief Return the subtraction of other matrix and current matrix
         * @param p_other : The matrix you want to subtract to the current one
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr Matrix4<T> operator-(const Matrix4<U>& p_other) const;

        /**
         * @brief Subtract other matrix to the current matrix
         * @param p_other : The matrix you want to subtract to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix4<T>& operator-=(const Matrix4<U>& p_other);

#pragma endregion
#pragma region Multiply

        /**
         * @brief Multiply scalar to elements
         * @param p_scalar : The value you want to scale the matrix with
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix4<T>& Multiply(U p_scalar);

        /**
         * @brief Multiply scalar to matrix left
         * @param p_left : Multiply this matrix by the other parameter
         * @param p_right : Multiply this scalar to the other parameter
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix4<T> Multiply(const Matrix4<T>& p_left, U p_right);

        /**
         * @brief Multiply matrix with another matrix.
         * @param p_other : The matrix you want to use as a scalar
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix4<T>& Multiply(const Matrix4<U>& p_other);

        /**
         * @brief Multiply matrix to matrix left
         * @param p_left : Multiply this matrix by the other parameter
         * @param p_right : Multiply this matrix to the other parameter
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix4<T> Multiply(const Matrix4<T>& p_left, const Matrix4<U>& p_right);

        template<typename U>
        Vector4<U> Multiply(const Vector4<U>& p_other);

        /**
        * @brief Multiply matrix to the current matrix
        * @param p_other : Multiply this matrix to the current one
        * @return The copy of the Matrix operation result
        */
        template<typename U>
        constexpr Matrix4<T> operator*(const Matrix4<U>& p_other) const;

        /**
        * @brief Multiply matrix to the current matrix
        * @param p_other : Multiply this matrix to the current one
        * @return The current Matrix modified
        */
        template<typename U>
        Matrix4<T>& operator*=(const Matrix4<U>& p_other);



#pragma endregion 

        // static Matrix4<T> Multiply(const Matrix4<T>& p_matrix, const Matrix4<T>& p_other);
        template<typename U>
        static Vector4<T> Multiply(const Matrix4<U>& p_matrix, const Vector4<T>& p_vector);
        [[nodiscard]] bool Equals(const Matrix4<T>& p_other) const;
        template<typename U>
        static void Set(Matrix4<T>& p_matrix, const Matrix4<U>& p_other);

#pragma endregion 

#pragma region Operators

        //operators
        Vector4<T> operator*(const Vector4<T>& p_vector);

        bool operator==(const Matrix4<T>& p_matrix) const;
        bool operator!=(const Matrix4<T>& p_matrix) const;

		Matrix4<T>& operator=(const Matrix4<T>& p_matrix);
		Matrix4<T>& operator=(Matrix4<T>&& p_matrix) noexcept;
    	
        template<typename U>
        Matrix4<T>& operator=(const Matrix4<U>& p_matrix);
    	
        T& operator[](const int p_index);
        T& operator()(const int p_row, const int p_col);

#pragma endregion

    };

	template<typename T>
	constexpr std::ostream& operator<<(std::ostream& p_os, const Matrix4<T>& p_matrix);

    using Matrix4F = Matrix4<float>;
    using Matrix4L = Matrix4<long>;
    using Matrix4U = Matrix4<unsigned int>;
    using Matrix4I = Matrix4<int>;
    using Matrix4D = Matrix4<double>;
}

#include <GPM/Matrix/Matrix4.inl>