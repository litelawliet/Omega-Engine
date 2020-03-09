#pragma once

#include <string>
#include <sstream>

using namespace GPM;

#pragma region Static Properties

template<typename T>
Matrix3<T> Matrix3<T>::identity = { 1,0,0,
									0,1,0,
									0,0,1 };
template<typename T>
Matrix3<T> Matrix3<T>::zero = { 0,0,0,
								0,0,0,
								0,0,0 };
#pragma endregion

#pragma region Constructors & Assignment

template<typename T>
constexpr Matrix3<T>::Matrix3()
{
	memcpy(m_data, identity.m_data, 9 * sizeof(T));
}

template<typename T>
constexpr Matrix3<T>::Matrix3(const T p_i0, const T p_i1, const T p_i2,
	const T p_i3, const T p_i4, const T p_i5,
	const T p_i6, const T p_i7, const T p_i8)
	: m_data{ p_i0, p_i1, p_i2,
			  p_i3, p_i4, p_i5,
			  p_i6, p_i7, p_i8 } {}

template<typename T>
constexpr Matrix3<T>::Matrix3(const T p_data[9])
{
	memcpy(m_data, p_data, 9 * sizeof(T));
}

template<typename T>
constexpr Matrix3<T>::Matrix3(const Matrix3& p_other)
{
	memcpy(m_data, p_other.m_data, 9 * sizeof(T));
}

template<typename T>
template<typename U>
constexpr Matrix3<T>::Matrix3(const Matrix3<U>& p_other)
{
	for (unsigned int i = 0; i < 9; ++i)
		m_data[i] = p_other.m_data[i];
}

template<typename T>
constexpr Matrix3<T>::Matrix3(Matrix3&& p_other) noexcept
{
	memcpy(m_data, p_other.m_data, 9 * sizeof(T));
}

template<typename T>
constexpr Matrix3<T>& Matrix3<T>::operator=(const Matrix3<T>& p_other)
{
	for (unsigned int i = 0; i < 9; ++i)
		m_data[i] = p_other.m_data[i];

	return *this;
}

template<typename T>
template<typename U>
constexpr Matrix3<T>& Matrix3<T>::operator=(const Matrix3<U>& p_other)
{
	for (unsigned int i = 0; i < 9; ++i)
		m_data[i] = static_cast<const T>(p_other.m_data[i]);

	return *this;
}

template<typename T> constexpr Matrix3<T>& Matrix3<T>::operator=(Matrix3<T>&& p_other) noexcept
{
	for (unsigned int i = 0; i < 9; ++i)
		m_data[i] = p_other.m_data[i];

	return *this;
}

template<typename T>
template<typename U>
constexpr Matrix3<T>& Matrix3<T>::operator=(Matrix3<U>&& p_other) noexcept
{
	for (unsigned int i = 0; i < 9; ++i)
		m_data[i] = p_other.m_data[i];

	return *this;
}


#pragma endregion

#pragma region Properties

template<typename T>
T Matrix3<T>::Determinant()
{
	return ((m_data[0] * ((m_data[4] * m_data[8]) - (m_data[5] * m_data[7])))
		- (m_data[1] * ((m_data[3] * m_data[8]) - (m_data[5] * m_data[6])))
		+ (m_data[2] * ((m_data[3] * m_data[7]) - (m_data[4] * m_data[6]))));
}

template<typename T>
T Matrix3<T>::Determinant(const Matrix3<T>& p_matrix3)
{
	return ((p_matrix3.m_data[0] * ((p_matrix3.m_data[4] * p_matrix3.m_data[8]) - (p_matrix3.m_data[5] * p_matrix3.m_data[7])))
		- (p_matrix3.m_data[1] * ((p_matrix3.m_data[3] * p_matrix3.m_data[8]) - (p_matrix3.m_data[5] * p_matrix3.m_data[6])))
		+ (p_matrix3.m_data[2] * ((p_matrix3.m_data[3] * p_matrix3.m_data[7]) - (p_matrix3.m_data[4] * p_matrix3.m_data[6]))));
}

template<typename T>
constexpr Matrix3<T>& Matrix3<T>::Transpose()
{
	Matrix3<T> tmpMat(this->m_data);

	for (int n = 0; n < 9; n++)
	{
		int i = n / 3;
		int j = n % 3;

		m_data[n] = tmpMat.m_data[3 * j + i];
	}
	return { *this };
}

template<typename T>
constexpr Matrix3<T> Matrix3<T>::Transpose(const Matrix3<T>& p_matrix3)
{
	Matrix3<T> tmpMat = identity;

	for (int n = 0; n < 9; n++)
	{
		int i = n / 3;
		int j = n % 3;

		tmpMat.m_data[n] = p_matrix3.m_data[3 * j + i];
	}

	return tmpMat;
}


template<typename T>
constexpr Vector3<T> Matrix3<T>::GetColumn(const int p_column)
{
	if (p_column < 0 || p_column > 3)
		throw std::logic_error("Matrix3::GetColumn() p_column is inferior to 0 or superior to 3. There are no columns at those indexes");


	return { m_data[p_column], m_data[p_column + 3], m_data[p_column + 6] };
}

template<typename T>
template<typename U>
constexpr void Matrix3<T>::SetColumn(const int p_column, const Vector3<U>& p_vector)
{
	if (p_column < 0 || p_column > 3)
		throw std::logic_error("Matrix3::SetColumn() p_column is inferior to 0 or superior to 3. There are no columns at those indexes");


	m_data[p_column] = p_vector.x;
	m_data[p_column + 3] = p_vector.y;
	m_data[p_column + 6] = p_vector.z;
}

template<typename T>
constexpr Vector3<T> Matrix3<T>::GetRow(const int p_row)
{
	if (p_row < 0 || p_row > 3)
		throw std::logic_error("Matrix3::GetRow() p_row is inferior to 0 or superior to 3. There are no rows at those indexes");

	return { m_data[4 * p_row], m_data[(4 * p_row) + 1], m_data[(4 * p_row) + 2] };
}

template<typename T>
template<typename U>
constexpr void Matrix3<T>::SetRow(const int p_row, const Vector3<U>& p_vector)
{
	if (p_row < 0 || p_row > 3)
		throw std::logic_error("Matrix3::SetRow() p_row is inferior to 0 or superior to 3. There are no rows at those indexes");

	m_data[p_row] = p_vector.x;
	m_data[p_row + 1] = p_vector.y;
	m_data[p_row + 2] = p_vector.z;
}

template<typename T>
constexpr Matrix3<T>& Matrix3<T>::Normalize()
{
	T determinant = Determinant();

	if (determinant == 0)
		throw std::logic_error("Matrix3::Normalize() : Cannot divide by 0. Determinant was 0");

	for (auto& i : m_data)
	{
		i /= determinant;
	}
	return { *this };
}

template<typename T>
constexpr Matrix3<T> Matrix3<T>::Normalize(const Matrix3<T>& p_matrix)
{
	return p_matrix.Normalize();
}

template<typename T>
template<typename U>
constexpr Matrix3<T>& Matrix3<T>::Translate(const Vector2<U>& p_vector)
{
	m_data[2] = p_vector.x;
	m_data[5] = p_vector.y;
	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::CreateTranslation(const Vector2<U>& p_vector)
{
	Matrix3<T> tempMat = identity;
	tempMat.m_data[2] = p_vector.x;
	tempMat.m_data[5] = p_vector.y;
	return tempMat;
}

template<typename T> constexpr Matrix3<T>& Matrix3<T>::Rotate(const float p_angle)
{
	m_data[0] = Tools::Utils::CosF(p_angle);
	m_data[1] = Tools::Utils::SinF(p_angle);
	m_data[3] = -Tools::Utils::SinF(p_angle);
	m_data[4] = Tools::Utils::CosF(p_angle);
	return { *this };
}

template<typename T>
constexpr Matrix3<T> Matrix3<T>::CreateRotation(const float p_angle)
{
	Matrix3<T> tempMat = identity;
	tempMat.m_data[0] = Tools::Utils::Cos(p_angle);
	tempMat.m_data[1] = Tools::Utils::Sin(p_angle);
	tempMat.m_data[3] = -Tools::Utils::Sin(p_angle);
	tempMat.m_data[4] = Tools::Utils::Cos(p_angle);
	return tempMat;
}

template<typename T>
template<typename U>
constexpr Matrix3<T>& Matrix3<T>::Scale(const Vector2<U>& p_vector)
{
	m_data[0] = p_vector.x;
	m_data[4] = p_vector.y;
	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::CreateScaling(const Vector2<U>& p_vector)
{
	Matrix3<T> tempMat = identity;
	tempMat.m_data[0] = p_vector.x;
	tempMat.m_data[4] = p_vector.y;
	return tempMat;
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::CreateTransformation(const Vector2<U>& p_pos, const float p_angle, const Vector2<U>& p_scale)
{
	static_assert(!std::is_integral<T>::value, "Matrix3::CreateTransformation : Can't do Transform Matrices with Matrix3<int>, as values would be rounded to 0");
	Matrix3<T> tempMat = identity;
	Matrix3<T> t = CreateTranslation(p_pos);
	Matrix3<T> r = CreateRotation(p_angle);
	Matrix3<T> s = CreateScaling(p_scale);
	tempMat = t * r * s;
	return tempMat;
}

#pragma endregion

#pragma region Arithmetic Operations

#pragma region Add

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::Add(const Matrix3<U>& p_other)
{
	for (unsigned int i = 0; i < 9; ++i)
		m_data[i] += p_other.m_data[i];

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::Add(const Matrix3<T>& p_left, const Matrix3<U>& p_right)
{
	return Matrix3<T>(p_left).Add(p_right);
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::operator+(const Matrix3<U>& p_other) const
{
	return Add(*this, p_other);
}

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::operator+=(const Matrix3<U>& p_other)
{
	return Add(p_other);
}

#pragma endregion 

#pragma region Subtract

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::Subtract(const Matrix3<U>& p_other)
{
	for (unsigned int i = 0; i < 9; ++i)
		m_data[i] -= static_cast<const T>(p_other.m_data[i]);

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::Subtract(const Matrix3<T>& p_left, const Matrix3<U>& p_right)
{
	return Matrix3<T>(p_left).Subtract(p_right);
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::operator-(const Matrix3<U>& p_other) const
{
	return Subtract(*this, p_other);
}

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::operator-=(const Matrix3<U>& p_other)
{
	return Subtract(p_other);
}

#pragma endregion

#pragma region Multiply

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::Multiply(U p_scalar)
{
	for (auto& val : m_data)
		val *= p_scalar;

	return { *this };
}

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::Multiply(const Matrix3<U>& p_other)
{
	Matrix3<T> tmpMat(this->m_data);

	for (unsigned int i = 0; i < 9; i += 3)
	{
		for (unsigned int j = 0; j < 3; j++)
		{
			m_data[i + j] = (tmpMat.m_data[i] * p_other.m_data[j])
				+ (tmpMat.m_data[i + 1] * p_other.m_data[j + 3])
				+ (tmpMat.m_data[i + 2] * p_other.m_data[j + 6]);
		}
	}
	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::Multiply(const Matrix3<T>& p_left, U p_scalar)
{
	return Matrix3<T>(p_left).Multiply(p_scalar);
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::Multiply(const Matrix3<T>& p_left, const Matrix3<U>& p_right)
{
	return Matrix3<T>(p_left).Multiply(p_right);
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::operator*(const Matrix3<U>& p_other) const
{
	return Matrix3<T>(*this).Multiply(p_other);
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::operator*(const U p_scalar) const
{
	return Matrix3<T>(*this).Multiply(p_scalar);
}

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::operator*=(const U p_scalar)
{
	return  { Multiply(p_scalar) };
}

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::operator*=(const Matrix3<U>& p_other)
{
	return  { Multiply(p_other) };
}

#pragma endregion

#pragma region Divide

template<typename T>
template<typename U>
Matrix3<T>& Matrix3<T>::Divide(U p_scalar)
{
	for (auto& val : m_data)
		val /= p_scalar;

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix3<T> Matrix3<T>::Divide(const Matrix3<T>& p_left, const U p_scalar)
{
	return p_left.Divide(p_scalar);
}

template<typename T>
template<class U>
constexpr Matrix3<T> Matrix3<T>::operator/(const U p_scalar) const
{
	return Matrix3<T>(*this).Divide(p_scalar);
}

template<typename T>
template<class U>
Matrix3<T>& Matrix3<T>::operator/=(const U p_scalar)
{
	return { Divide(p_scalar) };
}


#pragma endregion

#pragma endregion 

#pragma region Conversions

template <typename T>
constexpr std::string Matrix3<T>::ToString()
{
	std::stringstream stringStream;
	stringStream << '[' << m_data[0] << "  " << m_data[1] << "  " << m_data[2] << "]\n"
		<< '|' << m_data[3] << "  " << m_data[4] << "  " << m_data[5] << "|\n"
		<< '[' << m_data[6] << "  " << m_data[7] << "  " << m_data[8] << "]\n";
	return { stringStream.str() };
}

template<typename T>
constexpr std::string Matrix3<T>::ToString(const Matrix3<T>& p_matrix)
{
	std::stringstream stringStream;
	stringStream << '[' << p_matrix.m_data[0] << "  " << p_matrix.m_data[1] << "  " << p_matrix.m_data[2] << "]\n"
		<< '|' << p_matrix.m_data[3] << "  " << p_matrix.m_data[4] << "  " << p_matrix.m_data[5] << "|\n"
		<< '[' << p_matrix.m_data[6] << "  " << p_matrix.m_data[7] << "  " << p_matrix.m_data[8] << "]\n";
	return { stringStream.str() };
}

template<typename T>
T& Matrix3<T>::operator[](const int p_index)
{
	if (p_index < 0 || p_index > 8)
		throw std::out_of_range("Out of range index in Matrix3");

	return m_data[p_index];
}

template <typename T>
T& Matrix3<T>::operator()(const int p_row, const int p_col)
{
	if (p_row < 0 || p_row > 2)
		throw std::out_of_range("Out of range 'row' in Matrix3");
	else if (p_col < 0 || p_col > 2)
		throw std::out_of_range("Out of range 'col' in Matrix3");

	return m_data[p_row * 3 + p_col];
}

#pragma  endregion

#pragma region Tests & Comparisons


template<typename T>
constexpr bool Matrix3<T>::Equals(const Matrix3<T>& p_other) const
{
	for (unsigned int i = 0; i < 9; i++)
	{
		if (m_data[i] != p_other.m_data[i])
			return false;
	}
	return true;
}

template<typename T>
constexpr bool Matrix3<T>::AreEqual(const Matrix3<T>& p_left, const Matrix3<T>& p_right)
{
	return p_left.Equals(p_right);
}

template<typename T>
constexpr bool Matrix3<T>::IsIdentity(const Matrix3<T>& p_other)
{
	return p_other.Equals(identity);
}

template<typename T>
constexpr bool Matrix3<T>::operator==(const Matrix3<T>& p_other) const
{
	return Equals(p_other);
}

template<typename T>
constexpr bool Matrix3<T>::operator!=(const Matrix3<T>& p_other) const
{
	return !Equals(p_other);
}

#pragma endregion

#pragma region Outside Operators

template<typename T>
constexpr std::ostream& GPM::operator<<(std::ostream& p_stream, const Matrix3<T>& p_matrix3)
{
	p_stream << Matrix3<T>::ToString(p_matrix3);
	return p_stream;
}

#pragma endregion
