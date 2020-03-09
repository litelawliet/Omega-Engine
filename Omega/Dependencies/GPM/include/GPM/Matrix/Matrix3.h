#pragma once

#include <string>

namespace GPM
{
    /**
     * A standard 3 by 3 Matrix. Default value is an identity matrix
     */
    template <typename T>
    struct Matrix3
    {
        static_assert(std::is_arithmetic<T>::value, "Matrix3 should only be used with arithmetic types");

        T m_data[9] = { 1,0,0,
                        0,1,0,
                        0,0,1 };
#pragma region Constructors & Assignment

        /**
         * @brief Default Constructor
         */
        constexpr Matrix3();

        /**
         * @brief Default Destructor
         */
        ~Matrix3() = default;

        /**
         * @brief Constructor with parameters
         * @param p_i0 1st element
         * @param p_i1 2nd element
         * @param p_i2 3rd element
         * @param p_i3 4th element
         * @param p_i4 5th element
         * @param p_i5 6th element
         * @param p_i6 7th element
         * @param p_i7 8th element
         * @param p_i8 9th element
         */
        constexpr Matrix3(const T p_i0, const T p_i1, const T p_i2, 
                          const T p_i3, const T p_i4, const T p_i5,
                          const T p_i6, const T p_i7, const T p_i8);

        /**
         * @brief Constructor from already created array 
         * @param p_data : Array with a size of 9 elements
         */
        constexpr Matrix3(const T p_data[9]);

        /**
         * @brief Copy Constructor
         * @param p_other : Matrix to construct from
         */
        constexpr Matrix3(const Matrix3& p_other);

        /**
         * @brief Template Copy Constructor
         * @param p_other : Matrix to construct from
         */
        template<typename U>
        constexpr Matrix3(const Matrix3<U>& p_other);

        /**
         * @brief Move Constructor
         * @param p_other : Matrix to construct from
         */
        constexpr Matrix3(Matrix3&& p_other) noexcept;

        /**
        * @brief Assignment operator for Matrix3
        * @param p_other The matrix to construct from
        */
        constexpr Matrix3<T>& operator=(const Matrix3<T>& p_other);

        /**
        * @brief Assignment operator for Matrix3
        * @param p_other The matrix to construct from
        */
        template<typename U>
        constexpr Matrix3<T>& operator=(const Matrix3<U>& p_other);

         /**
         * @brief Assignment move operator for Matrix3
         * @param p_other The matrix to construct from
         */
        constexpr Matrix3<T>& operator=(Matrix3<T>&& p_other) noexcept;

        /**
         * @brief Assignment move operator for Matrix3
         * @param p_other The matrix to construct from
         */
        template<typename U>
        constexpr Matrix3<T>& operator=(Matrix3<U>&& p_other) noexcept;


#pragma endregion

#pragma region Static Properties
        /**
         * @brief This is a const matrix that has 1 in the diagonal and 0 elsewhere
         */
        static Matrix3<T> identity;

        /** 
         *  @brief This is a const matrix that has 0 everywhere
         */
        static Matrix3<T> zero;
#pragma endregion 

#pragma region Properties

        /**
         * @brief Calculates the determinant of the matrix
         * @return The determinant in any arithmetic type you want
         */
        T Determinant();

        /**
         * @brief Calculates the determinant of the matrix
         * @param p_matrix3 : The matrix to get the Determinant from
         * @return The determinant in any arithmetic type you want
         */
        static T Determinant(const Matrix3<T>& p_matrix3);

        /**
         * @brief Returns this matrix transposed
         * @return The same matrix that has its columns exchanged with its rows
         */
        constexpr Matrix3<T>& Transpose();

        /**
         * @brief Returns this matrix transposed
         * @param p_matrix3 : The matrix to get the Transpose from
         * @return The same matrix that has its columns exchanged with its rows
         */
        constexpr static Matrix3<T> Transpose(const Matrix3<T>& p_matrix3);

        /**
         * @brief Attempts to retrieve a column from the matrix.
         * @param p_column : Which column to retrieve
         * @return The column in a Vector type
         */
        constexpr Vector3<T> GetColumn(const int p_column);

        /**
         * @brief Attempts to set a column in the matrix.
         * @param p_column : Which column to retrieve
         * @param p_vector : The values to set in the column
         */
        template<typename U>
        constexpr void SetColumn(const int p_column, const Vector3<U>& p_vector);

        /**
         * @brief Attempts to retrieve a row from the matrix.
         * @param p_row : Which row to retrieve
         * @return The row in a Vector type
         */
        constexpr Vector3<T> GetRow(const int p_row);

        /**
         * @brief Attempts to set a row in the matrix.
         * @param p_row : Which row to retrieve
         * @param p_vector : The values to set in the row
         */
        template<typename U>
        constexpr void SetRow(const int p_row, const Vector3<U>& p_vector);

        /**
         * @brief Returns this matrix with a magnitude of 1
         * @return The modified matrix with a magnitude of 1
         */
        constexpr Matrix3<T>& Normalize();

        /**
         * @brief Returns this matrix with a magnitude of 1
         * @return The modified matrix with a magnitude of 1
         */
        constexpr static Matrix3<T> Normalize(const Matrix3<T>& p_matrix);

        /**
         * @brief Translates the current matrix with a Vector2
         * @param p_vector : The desired 2D off-set
         * @return The current matrix translated
         */
        template<typename U>
        constexpr Matrix3<T>& Translate(const Vector2<U>& p_vector);

        /**
         * @brief Creates a translation identity matrix with a Vector2 offset
         * @param p_vector : The desired 2D off-set
         * @return The translated identity matrix
         */
        template<typename U>
        constexpr static Matrix3<T> CreateTranslation(const Vector2<U>& p_vector);

        /**
         * @brief Rotate the current matrix with an angle in degrees
         * @param p_angle : The desired angle in degrees to rotate the matrix
         * @return The current matrix rotated
         */
        constexpr Matrix3<T>& Rotate(const float p_angle);

        /**
         * @brief Creates a rotation identity matrix with an angle in degrees
         * @param p_angle : The desired angle in degrees to rotate the matrix
         * @return The rotated identity matrix
         */
        constexpr static Matrix3<T> CreateRotation(const float p_angle);

        /**
         * @brief Scale the current matrix with a Vector2
         * @param p_vector : The desired 2D Scale
         * @return The current matrix scaled
         */
        template<typename U>
        constexpr Matrix3<T>& Scale(const Vector2<U>& p_vector);

        /**
         * @brief Creates a scaling identity matrix with a Vector2
         * @param p_vector : the desired 2D scale
         * @return The scaled identity matrix
         */
        template<typename U>
        constexpr static Matrix3<T> CreateScaling(const Vector2<U>& p_vector);

        /**
         * @brief Creates a translation, rotation and scaling matrix
         * @param p_pos : The position to set the matrix to
         * @param p_angle : The rotation to orient the matrix to
         * @param p_scale : The scale to set the matrix to
         * @return A translation matrix set with the given parameters
         */
        template<typename U>
        constexpr static Matrix3<T> CreateTransformation(const Vector2<U>& p_pos, const float p_angle, const Vector2<U>& p_scale);

        /**
         * @brief Computes a transformation matrix that looks at a target
         * @param 
         * @param 
         * @param
         * @return the modified matrix looking at the target
         *
         * @warning /!\ W.I.P. NOT FUNCTIONAL AS OF NOW /!\
         */
        template<typename U>
        constexpr Matrix3<T>& LookAt(const Vector3<U>& p_dir, const Vector3<U>& p_up, Matrix3<T>& p_matrix);
#pragma endregion

#pragma region Arithmetic Operations

#pragma region Add

        /**
         * @brief Add other matrix to the current matrix
         * @param p_other : The other matrix you want to add to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix3<T>& Add(const Matrix3<U>& p_other);

        /**
         * @brief Add left matrix to the right matrix
         * @param p_left : The first matrix you want to add
         * @param p_right : The second matrix to be added to the first one
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix3<T> Add(const Matrix3<T>& p_left, const Matrix3<U>& p_right);

        /**
         * @brief Return the summation of other matrix and current matrix
         * @param p_other : The other matrix you want to add
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr Matrix3<T> operator+(const Matrix3<U>& p_other) const;

        /**
         * @brief Add other matrix to the current matrix
         * @param p_other : The other matrix you want to add to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix3<T>& operator+=(const Matrix3<U>& p_other);

#pragma endregion
#pragma region Subtract

        /**
         * @brief Subtract other matrix to the current matrix
         * @param p_other : The matrix you want to subtract to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix3<T>& Subtract(const Matrix3<U>& p_other);

        /**
         * @brief Subtract left matrix to the right matrix
         * @param p_left : The first matrix you want to subtract to
         * @param p_right : The second matrix to be subtracted from the first one
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix3<T> Subtract(const Matrix3<T>& p_left, const Matrix3<U>& p_right);

        /**
         * @brief Return the subtraction of other matrix and current matrix
         * @param p_other : The matrix you want to subtract to the current one
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr Matrix3<T> operator-(const Matrix3<U>& p_other) const;

        /**
         * @brief Subtract other matrix to the current matrix
         * @param p_other : The matrix you want to subtract to the current one
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix3<T>& operator -=(const Matrix3<U>& p_other);

#pragma endregion
#pragma region Multiply

        /**
         * @brief Multiply scalar to elements
         * @param p_scalar : The value you want to scale the matrix with
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix3<T>& Multiply(U p_scalar);

        /**
         * @brief Multiply matrix with another matrix.
         * @param p_other : The matrix you want to use as a scalar
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix3<T>& Multiply(const Matrix3<U>& p_other);

        /**
         * @brief Multiply scalar to matrix left
         * @param p_left : Multiply this matrix by the other parameter
         * @param p_scalar : Multiply this scalar to the other parameter
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix3<T> Multiply(const Matrix3<T>& p_left, U p_scalar);

        /**
         * @brief Multiply scalar to matrix left
         * @param p_left : Multiply this matrix by the other parameter
         * @param p_right : Multiply this matrix to the other parameter
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix3<T> Multiply(const Matrix3<T>& p_left, const Matrix3<U>& p_right);

        /**
        * @brief Multiply matrix to the current matrix
        * @param p_other : Multiply this matrix to the current one
        * @return The copy of the Matrix operation result
        */
        template<class U>
        constexpr Matrix3<T> operator*(const Matrix3<U>& p_other) const;

        /**
        * @brief Multiply matrix to the current matrix
        * @param p_scalar : Multiply this matrix to the scalar
        * @return The copy of the Matrix operation result
        */
        template<class U>
        constexpr Matrix3<T> operator*(const U p_scalar) const;

        /**
        * @brief Multiply matrix to the current matrix
        * @param p_scalar : Multiply this matrix to the scalar
        * @return The current Matrix modified
        */
        template<class U>
        Matrix3<T>& operator*=(const U p_scalar);
        
        /**
        * @brief Multiply matrix to the current matrix
        * @param p_other : Multiply this matrix to the current one
        * @return The current Matrix modified
        */
        template<class U>
        Matrix3<T>& operator*=(const Matrix3<U>& p_other);

#pragma endregion

#pragma region Divide

        /**
         * @brief Divide elements by scalar
         * @param p_scalar : The value you want to scale the matrix with
         * @return The current Matrix modified
         */
        template<typename U>
        Matrix3<T>& Divide(U p_scalar);

        /**
         * @brief Divide matrix p_left by scalar
         * @param p_left : Multiply this matrix by the other parameter
         * @param p_scalar : Multiply this scalar to the other parameter
         * @return The copy of the Matrix operation result
         */
        template<typename U>
        constexpr static Matrix3<T> Divide(const Matrix3<T>& p_left, const U p_scalar);
        
        /**
         * @brief Divide matrix p_left by scalar
         * @param p_scalar : the scalar to divide
         * @return The copy of the Matrix operation result
         */
        template<class U>
        constexpr Matrix3<T> operator/(const U p_scalar) const;

        /**
         * @brief Divide elements by scalar
         * @param p_scalar : The value you want to divide the matrix by
         * @return The current Matrix modified
         */
        template<class U>
        Matrix3<T>& operator/=(const U p_scalar);

#pragma endregion

#pragma endregion

#pragma region Tests & Comparisons
        /**
         * @brief Return true if the two matrices are identical
         * @param p_other The matrix used for the checkup
         * @return True or false
         */
        [[nodiscard]] constexpr bool Equals(const Matrix3<T>& p_other) const;

        /**
         * @brief Return true if the two matrices are identical
         * @param p_left The left matrix
         * @param p_right The right matrix
         * @return True or false
         */
        constexpr static bool AreEqual(const Matrix3<T>& p_left, const Matrix3<T>& p_right);

        /**
         * @brief Return true if the two matrices are identical
         * @param p_other The matrix used for the checkup
         * @return True or false
         */
        constexpr static bool IsIdentity(const Matrix3<T>& p_other);

        /**
         * @brief Return true if the two matrices are identical
         * @param p_other The matrix used for the checkup
         * @return True or false
         */
        constexpr bool operator==(const Matrix3<T>& p_other) const;

        /**
         * @brief Return true if the two matrices are different
         * @param p_other The matrix used for the checkup
         * @return True or false
         */
        constexpr bool operator!=(const Matrix3<T>& p_other) const;
#pragma endregion

#pragma region Conversions
        /**
         * @brief function used to debug values from the matrix
         */
        constexpr std::string ToString();

        /**
        * @brief Function used to debug values from the matrix
        * @param p_matrix The matrix to print
        */
        constexpr static std::string ToString(const Matrix3<T>& p_matrix);

    	T& operator[](const int p_index);
    	T& operator()(const int p_row, const int p_col);

#pragma endregion
    };// end struct Matrix3

    
#pragma region Outside Operators

    template <typename T>
    constexpr std::ostream& operator<<(std::ostream& p_stream, const Matrix3<T>& p_matrix3);
#pragma endregion

    using Matrix3F = Matrix3<float>;
    using Matrix3L = Matrix3<long>;
    using Matrix3I = Matrix3<int>;
    using Matrix3D = Matrix3<double>;
}// end Namespace GPM

#include <GPM/Matrix/Matrix3.inl>