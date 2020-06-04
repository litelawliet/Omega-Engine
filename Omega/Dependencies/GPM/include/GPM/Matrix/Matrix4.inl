#pragma once
#include <GPM/Quaternion/Quaternion.h>
#include <stdexcept>

// struct GPM::Quaternion;

#pragma region Static Properties

template<typename T>
Matrix4<T> Matrix4<T>::identity = { 1,0,0,0,
									0,1,0,0,
									0,0,1,0,
									0,0,0,1 };
template<typename T>
Matrix4<T> Matrix4<T>::zero = { 0,0,0,0,
								0,0,0,0,
								0,0,0,0,
								0,0,0,0 };
#pragma endregion

#pragma region Constuctor

template<typename T>
constexpr Matrix4<T>::Matrix4()
{
	memcpy_s(m_data, 16 * sizeof(T), identity.m_data, 16 * sizeof(T));
}

template<typename T>
constexpr Matrix4<T>::Matrix4(const Matrix4<T>& p_matrix)
{
	memcpy_s(m_data, 16 * sizeof(T), p_matrix.m_data, 16 * sizeof(T));
}


template<typename T>
Matrix4<T>::Matrix4(Matrix4<T>&& p_matrix) noexcept
{
	memcpy_s(m_data, 16 * sizeof(T), p_matrix.m_data, 16 * sizeof(T));
}

template<typename T>
constexpr Matrix4<T>::Matrix4(const Vector3<T>& p_vector)
{
	m_data[0] = p_vector.x;
	m_data[5] = p_vector.y;
	m_data[10] = p_vector.z;
}

template<typename T>
constexpr Matrix4<T>::Matrix4(const T p_data[16])
{
	if (p_data == nullptr)
		return;

	memcpy_s(m_data, 16 * sizeof(T), p_data, 16 * sizeof(T));
}

template<typename T>
constexpr Matrix4<T>::Matrix4(const T p_00, const T p_01, const T p_02, const T p_03,
	const T p_10, const T p_11, const T p_12, const T p_13,
	const T p_20, const T p_21, const T p_22, const T p_23,
	const T p_30, const T p_31, const T p_32, const T p_33)
	: m_data{ p_00, p_01, p_02, p_03, p_10, p_11, p_12, p_13, p_20, p_21, p_22, p_23, p_30, p_31, p_32, p_33 }
{
}

#pragma endregion

#pragma region Properties

template<typename T>
constexpr bool Matrix4<T>::isIdentity()
{
	return Equals(identity);
}

template<typename T>
constexpr T Matrix4<T>::Determinant()
{
	return ((m_data[0] * GetMinor({ m_data[5], m_data[6], m_data[7], m_data[9], m_data[10], m_data[11], m_data[13], m_data[14], m_data[15] }))
		- (m_data[1] * GetMinor({ m_data[4], m_data[6], m_data[7], m_data[8], m_data[10], m_data[11], m_data[12], m_data[14], m_data[15] }))
		+ (m_data[2] * GetMinor({ m_data[4], m_data[5], m_data[7], m_data[8], m_data[9], m_data[11], m_data[12], m_data[13], m_data[15] }))
		- (m_data[3] * GetMinor({ m_data[4], m_data[5], m_data[6], m_data[8], m_data[9], m_data[10], m_data[12], m_data[13], m_data[14] })));
}

template<typename T>
constexpr T Matrix4<T>::Determinant(const Matrix4<T>& p_matrix)
{
	return Matrix4<T>(p_matrix).Determinant();
}

template<typename T>
constexpr Matrix4<T>& Matrix4<T>::Transpose()
{
	Matrix4<T> tmpMat(this->m_data);

	for (int n = 0; n < 16; n++)
	{
		int i = n / 4;
		int j = n % 4;

		m_data[n] = tmpMat.m_data[4 * j + i];
	}

	return { *this };
}


template<typename T>
constexpr Matrix4<T> Matrix4<T>::Transpose(const Matrix4<T>& p_matrix)
{
	Matrix4<T> tmpMat = identity;

	for (uint64_t i = 0; i < 4; ++i)
	{
		for (uint64_t j = 0; j < 4; ++j)
		{
			tmpMat.m_data(i, j) = p_matrix.m_data(j , i);
		}
	}

	return tmpMat;
}

template<typename T>
constexpr Matrix4<T>& Matrix4<T>::Normalize()
{
	T det = Determinant();

	for (auto& i : m_data)
	{
		i /= det;
	}

	return { *this };
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::Normalize(const Matrix4<T>& p_matrix)
{
	return p_matrix.Normalize();
}


template<typename T>
template<typename U>
constexpr Matrix4<T>& Matrix4<T>::Scale(const Vector3<U>& p_scale)
{
	(*this) *= CreateScale(p_scale);

	return { (*this) };
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::CreateScale(const Vector3<U>& p_scale)
{
	Matrix4<T> tmpScale = identity;
	tmpScale.m_data[0] = p_scale.x;
	tmpScale.m_data[5] = p_scale.y;
	tmpScale.m_data[10] = p_scale.z;

	return tmpScale;
}

template<typename T>
constexpr Matrix4<T>& Matrix4<T>::Rotate(const Quaternion& p_rotation)
{
	(*this) *= p_rotation.ToMatrix4();

	return { *this };
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::CreateRotation(const Quaternion& p_rotation)
{
	return p_rotation.ToMatrix4();
}

template<typename T>
template<typename U>
constexpr Matrix4<T>& Matrix4<T>::Translate(const Vector3<U>& p_translate)
{
	(*this) *= CreateTranslation(p_translate);

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::CreateTranslation(const Vector3<U>& p_translate)
{
	Matrix4<T> tmpTrans = identity;

	tmpTrans.m_data[3] = p_translate.x;
	tmpTrans.m_data[7] = p_translate.y;
	tmpTrans.m_data[11] = p_translate.z;

	return tmpTrans;
}

template<typename T>
template<typename U>
constexpr Matrix4<T>& Matrix4<T>::Transform(const Vector3<U>& p_translate, const Quaternion& p_rotation, const Vector3<U>& p_scale)
{
	(*this) *= CreateTransformation(p_translate, p_rotation, p_scale);

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::CreateTransformation(const Vector3<U>& p_translate, const Quaternion& p_rotation, const Vector3<U>& p_scale)
{
	Matrix4<T> tmpTrans = CreateTranslation(p_translate);
	Matrix4<T> tmpRot = CreateRotation(p_rotation);
	Matrix4<T> tmpScale = CreateScale(p_scale);

	Matrix4<T> tmpMat = tmpTrans * tmpRot * tmpScale;

	return tmpMat;
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::Adjugate()
{
	Matrix4<T> tmpMat(m_data);

	tmpMat.m_data[0] = GetMinor({ m_data[5], m_data[6], m_data[7], m_data[9], m_data[10], m_data[11], m_data[13], m_data[14], m_data[15] });
	tmpMat.m_data[4] = -GetMinor({ m_data[4], m_data[6], m_data[7], m_data[8], m_data[10], m_data[11], m_data[12], m_data[14], m_data[15] });
	tmpMat.m_data[8] = GetMinor({ m_data[4], m_data[5], m_data[7], m_data[8], m_data[9], m_data[11], m_data[12], m_data[13], m_data[15] });
	tmpMat.m_data[12] = -GetMinor({ m_data[4], m_data[5], m_data[6], m_data[8], m_data[9], m_data[10], m_data[12], m_data[13], m_data[14] });

	tmpMat.m_data[1] = -GetMinor({ m_data[1], m_data[2], m_data[3], m_data[9], m_data[10], m_data[11], m_data[13], m_data[14], m_data[15] });
	tmpMat.m_data[5] = GetMinor({ m_data[0], m_data[2], m_data[3], m_data[8], m_data[10], m_data[11], m_data[12], m_data[14], m_data[15] });
	tmpMat.m_data[9] = -GetMinor({ m_data[0], m_data[1], m_data[3], m_data[8], m_data[9], m_data[11], m_data[12], m_data[13], m_data[15] });
	tmpMat.m_data[13] = GetMinor({ m_data[0], m_data[1], m_data[2], m_data[8], m_data[9], m_data[10], m_data[12], m_data[13], m_data[14] });

	tmpMat.m_data[2] = GetMinor({ m_data[1], m_data[2], m_data[3], m_data[5], m_data[6], m_data[7], m_data[13], m_data[14], m_data[15] });
	tmpMat.m_data[6] = -GetMinor({ m_data[0], m_data[2], m_data[3], m_data[4], m_data[6], m_data[7], m_data[12], m_data[14], m_data[15] });
	tmpMat.m_data[10] = GetMinor({ m_data[0], m_data[1], m_data[3], m_data[4], m_data[5], m_data[7], m_data[12], m_data[13], m_data[15] });
	tmpMat.m_data[14] = -GetMinor({ m_data[0], m_data[1], m_data[2], m_data[4], m_data[5], m_data[6], m_data[12], m_data[13], m_data[14] });

	tmpMat.m_data[3] = -GetMinor({ m_data[1], m_data[2], m_data[3], m_data[5], m_data[6], m_data[7], m_data[9], m_data[10], m_data[11] });
	tmpMat.m_data[7] = GetMinor({ m_data[0], m_data[2], m_data[3], m_data[4], m_data[6], m_data[7], m_data[8], m_data[10], m_data[11] });
	tmpMat.m_data[11] = -GetMinor({ m_data[0], m_data[1], m_data[3], m_data[4], m_data[5], m_data[7], m_data[8], m_data[9], m_data[11] });
	tmpMat.m_data[15] = GetMinor({ m_data[0], m_data[1], m_data[2], m_data[4], m_data[5], m_data[6], m_data[8], m_data[9], m_data[10] });

	return { tmpMat };
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::CreateAdjugate(const Matrix4<T>& p_matrix)
{
	return Matrix4<T>(p_matrix).Adjugate();
}


template<typename T>
constexpr Matrix4<T> Matrix4<T>::Inverse(const Matrix4<T>& p_matrix)
{
	static_assert(!std::is_integral<T>::value, "Matrix3::CreateTransformation : Can't do Transform Matrices with Matrix3<int>, as values would be rounded to 0");

	Matrix4<T> tmpMat;
	const Matrix4<T> adj = CreateAdjugate(p_matrix);
	const T det = Determinant(p_matrix);

	for (uint64_t i = 0u; i < 16u; ++i)
	{
		tmpMat.m_data[i] = adj.m_data[i] / static_cast<T>(det);
	}

	return tmpMat;
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::Rotate(const Matrix4<T>& p_matrix, const T p_angle, const Vector3<T>& p_axis)
{
	T const a = p_angle;
	T const c = static_cast<T>(Tools::Utils::Cos(a));
	T const s = static_cast<T>(Tools::Utils::Sin(a));

	Vector3<T> axis(Vector3<T>::Normalize(p_axis));
	Vector3<T> temp(axis * (T(1) - c));

	Matrix4<T> rotate = identity;
	rotate(0, 0) = c + temp[0] * axis[0];
	rotate(0, 1) = temp[0] * axis[1] + s * axis[2];
	rotate(0, 2) = temp[0] * axis[2] - s * axis[1];

	rotate(1, 0) = temp[1] * axis[0] - s * axis[2];
	rotate(1, 1) = c + temp[1] * axis[1];
	rotate(1, 2) = temp[1] * axis[2] + s * axis[0];

	rotate(2, 0) = temp[2] * axis[0] + s * axis[1];
	rotate(2, 1) = temp[2] * axis[1] - s * axis[0];
	rotate(2, 2) = c + temp[2] * axis[2];

	Matrix4<T> Result;
	Vector4<T> firstLine = Vector4<T>{ p_matrix.m_data[0], p_matrix.m_data[1], p_matrix.m_data[2], p_matrix.m_data[3] };
	Vector4<T> secondLine = Vector4<T>{ p_matrix.m_data[4], p_matrix.m_data[5], p_matrix.m_data[6], p_matrix.m_data[7] };
	Vector4<T> thirdLine = Vector4<T>{ p_matrix.m_data[8], p_matrix.m_data[9], p_matrix.m_data[10], p_matrix.m_data[11] };
	Vector4<T> fourthLine = Vector4<T>{ p_matrix.m_data[12], p_matrix.m_data[13], p_matrix.m_data[14], p_matrix.m_data[15] };

	Vector4<T> resultFirst = firstLine * rotate(0, 0) + secondLine * rotate(0, 1) + thirdLine * rotate(0, 2);
	Vector4<T> resultSecond = firstLine * rotate(1, 0) + secondLine * rotate(1, 1) + thirdLine * rotate(1, 2);
	Vector4<T> resultThird = firstLine * rotate(2, 0) + secondLine * rotate(2, 1) + thirdLine * rotate(2, 2);

	Result[0] = resultFirst[0]; Result[1] = resultFirst[1]; Result[2] = resultFirst[2]; Result[3] = resultFirst[3];
	Result[4] = resultSecond[0]; Result[5] = resultSecond[1]; Result[6] = resultSecond[2]; Result[7] = resultSecond[3];
	Result[8] = resultThird[0]; Result[9] = resultThird[1]; Result[10] = resultThird[2]; Result[11] = resultThird[3];
	Result[12] = fourthLine[0]; Result[13] = fourthLine[1]; Result[14] = fourthLine[2]; Result[15] = fourthLine[3];

	return Result;
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::LookAt(const Vector3<T>& p_eye, const Vector3<T>& p_target, const Vector3<T>& p_up)
{
	const Vector3<T> forward = Vector3<T>::Normalize(p_eye - p_target);
	const Vector3<T> left = Vector3<T>::Normalize(Vector3<T>::Cross(p_up, forward));
	const Vector3<T> up = Vector3<T>::Cross(left, forward);

	Matrix4 matrix;
	memset(matrix.m_data, 0, sizeof(T) * 16);
	matrix = Matrix4::identity;

	// Rotation part
	matrix.m_data[0] = left.x;
	matrix.m_data[4] = left.y;
	matrix.m_data[8] = left.z;
	matrix.m_data[1] = up.x;
	matrix.m_data[5] = up.y;
	matrix.m_data[9] = up.z;
	matrix.m_data[2] = forward.x;
	matrix.m_data[6] = forward.y;
	matrix.m_data[10] = forward.z;

	// Translation part
	matrix.m_data[12] = -left.x * p_eye.x - left.y * p_eye.y - left.z * p_eye.z;
	matrix.m_data[13] = -up.x * p_eye.x - up.y * p_eye.y - up.z * p_eye.z;
	matrix.m_data[14] = -forward.x * p_eye.x - forward.y * p_eye.y - forward.z * p_eye.z;

	return matrix;
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::Perspective(const T p_fovy, const T p_aspectRatio, const T p_near, const T p_far)
{
	const T top = static_cast<T>(Tools::Utils::Tan(p_fovy * static_cast<T>(0.00872664625))* p_near);

	Matrix4<T> result;
	memset(result.m_data, 0, sizeof(T) * 16);


	result(0, 0) = p_near / (top * p_aspectRatio);
	result(1, 1) = p_near / top;
	result(2, 2) = -(p_far + p_near) / (p_far - p_near);
	result(2, 3) = -static_cast<T>(1);
	result(3, 2) = -(static_cast<T>(2)* p_far* p_near) / (p_far - p_near);

	return result;
}

template<typename T>
constexpr Matrix4<T> Matrix4<T>::Orthographic(const T p_left, const T p_right, const T p_bottom, const T p_top, const T p_near, const T p_far)
{
	Matrix4<T> result = identity;

	result(0, 0) = static_cast<T>(2) / static_cast<T>(p_right - p_left);
	result(1, 1) = static_cast<T>(2) / static_cast<T>(p_top - p_bottom);
	result(2, 2) = -static_cast<T>(1);
	result(3, 0) = -(p_right + p_left) / static_cast<T>(p_right - p_left);
	result(3, 1) = -(p_top + p_bottom) / static_cast<T>(p_top - p_bottom);
	result(3, 2) = -(p_far + p_near) / static_cast<T>(p_far - p_near);

	return result;
}

#pragma endregion 


template<typename T>
constexpr void Matrix4<T>::SetColumn(const int p_column, const Vector4<T>& p_vector)
{
	m_data[p_column] = p_vector.x;
	m_data[p_column + 4] = p_vector.y;
	m_data[p_column + 8] = p_vector.z;
	m_data[p_column + 12] = p_vector.w;
}

template<typename T>
constexpr void Matrix4<T>::SetRow(const int p_row, const Vector4<T>& p_vector)
{
	uint64_t rowStart = 4u * p_row;
	m_data[rowStart] = p_vector.x;
	m_data[rowStart + 1] = p_vector.y;
	m_data[rowStart + 2] = p_vector.z;
	m_data[rowStart + 3] = p_vector.w;
}

template<typename T>
T Matrix4<T>::GetMinor(Matrix3<T> p_minor)
{
	return ((p_minor.m_data[0] * ((p_minor.m_data[4] * p_minor.m_data[8]) - (p_minor.m_data[5] * p_minor.m_data[7])))
		- (p_minor.m_data[1] * ((p_minor.m_data[3] * p_minor.m_data[8]) - (p_minor.m_data[5] * p_minor.m_data[6])))
		+ (p_minor.m_data[2] * ((p_minor.m_data[3] * p_minor.m_data[7]) - (p_minor.m_data[4] * p_minor.m_data[6]))));
}

#pragma region Conversions

template<typename T>
constexpr std::string Matrix4<T>::ToString() const noexcept
{
	std::stringstream stringStream;
	stringStream << '[' << m_data[0] << "  " << m_data[1] << "  " << m_data[2] << "  " << m_data[3] << "]\n"
		<< '[' << m_data[4] << "  " << m_data[5] << "  " << m_data[6] << "  " << m_data[7] << "]\n"
		<< '[' << m_data[8] << "  " << m_data[9] << "  " << m_data[10] << "  " << m_data[11] << "]\n"
		<< '[' << m_data[12] << "  " << m_data[13] << "  " << m_data[14] << "  " << m_data[15] << "]\n";
	return { stringStream.str() };
}

template<typename T>
constexpr std::string Matrix4<T>::ToString(const Matrix4<T>& p_matrix)
{
	std::stringstream stringStream;
	stringStream << '[' << p_matrix.m_data[0] << "  " << p_matrix.m_data[1] << "  " << p_matrix.m_data[2] << "  " << p_matrix.m_data[3] << "]\n"
		<< '[' << p_matrix.m_data[4] << "  " << p_matrix.m_data[5] << "  " << p_matrix.m_data[6] << "  " << p_matrix.m_data[7] << "]\n"
		<< '[' << p_matrix.m_data[8] << "  " << p_matrix.m_data[9] << "  " << p_matrix.m_data[10] << "  " << p_matrix.m_data[11] << "]\n"
		<< '[' << p_matrix.m_data[12] << "  " << p_matrix.m_data[13] << "  " << p_matrix.m_data[14] << "  " << p_matrix.m_data[15] << "]\n";
	return { stringStream.str() };
}

#pragma endregion

#pragma region Arithmetic Operations

#pragma region Add

template<typename T>
template<typename U>
Matrix4<T>& Matrix4<T>::Add(const Matrix4<U>& p_other)
{
	for (uint64_t i = 0u; i < 16u; ++i)
	{
		m_data[i] += p_other.m_data[i];
	}

	return { *this };
}

template<typename T>
template<typename U>
constexpr  Matrix4<T> Matrix4<T>::Add(const Matrix4<T>& p_left, const Matrix4<U>& p_right)
{
	return Matrix4<T>(p_left).Add(p_right);
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::operator+(const Matrix4<U>& p_other) const
{
	return Add(*this, p_other);
}

template<typename T>
template<typename U>
Matrix4<T> Matrix4<T>::operator+=(const Matrix4<U>& p_other)
{
	return Add(p_other);
}

#pragma endregion

#pragma region Subtract

template<typename T>
template<typename U>
Matrix4<T> Matrix4<T>::Subtract(const Matrix4<U>& p_other)
{
	for (uint64_t i = 0u; i < 16u; ++i)
	{
		m_data[i] -= p_other.m_data[i];
	}

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::Subtract(const Matrix4<T>& p_left, const Matrix4<U>& p_right)
{
	return Matrix4<T>(p_left).Subtract(p_right);
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::operator-(const Matrix4<U>& p_other) const
{
	return Subtract(*this, p_other);
}

template<typename T>
template<typename U>
Matrix4<T>& Matrix4<T>::operator-=(const Matrix4<U>& p_other)
{
	return Subtract(p_other);
}


#pragma endregion

#pragma region Multiply

template<typename T>
template<typename U>
Matrix4<T>& Matrix4<T>::Multiply(U p_scalar)
{
	for (uint64_t i = 0u; i < 16u; ++i)
	{
		m_data[i] *= p_scalar;
	}

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::Multiply(const Matrix4<T>& p_left, U p_right)
{
	return Matrix4<T>(p_left).Multiply(p_right);
}


template<typename T>
template<typename U>
Matrix4<T>& Matrix4<T>::Multiply(const Matrix4<U>& p_other)
{
	Matrix4<T> tmpMat(this->m_data);

	for (uint64_t i = 0u; i < 16u; i += 4)
	{
		for (uint64_t j = 0u; j < 4u; j++)
		{
			m_data[i + j] = (tmpMat.m_data[i] * p_other.m_data[j])
				+ (tmpMat.m_data[i + 1] * p_other.m_data[j + 4])
				+ (tmpMat.m_data[i + 2] * p_other.m_data[j + 8])
				+ (tmpMat.m_data[i + 3] * p_other.m_data[j + 12]);
		}
	}

	return { *this };
}

template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::Multiply(const Matrix4<T>& p_left, const Matrix4<U>& p_right)
{
	return Matrix4<T>(p_left).Multiply(p_right);
}

template<typename T>
template<typename U>
Vector4<U> Matrix4<T>::Multiply(const Vector4<U>& p_other)
{
	Vector4<T> tmpVec = Vector4F::zero;

	tmpVec.x = (m_data[0] * p_other.x)
		+ (m_data[1] * p_other.y)
		+ (m_data[2] * p_other.z)
		+ (m_data[3] * p_other.w);

	tmpVec.y = (m_data[4] * p_other.x)
		+ (m_data[5] * p_other.y)
		+ (m_data[6] * p_other.z)
		+ (m_data[7] * p_other.w);

	tmpVec.z = (m_data[8] * p_other.x)
		+ (m_data[9] * p_other.y)
		+ (m_data[10] * p_other.z)
		+ (m_data[11] * p_other.w);

	tmpVec.w = (m_data[12] * p_other.x)
		+ (m_data[13] * p_other.y)
		+ (m_data[14] * p_other.z)
		+ (m_data[15] * p_other.w);

	return tmpVec;
}


template<typename T>
template<typename U>
constexpr Matrix4<T> Matrix4<T>::operator*(const Matrix4<U>& p_other) const
{
	return Matrix4<T>(*this).Multiply(p_other);
}

template<typename T>
template<typename U>
Matrix4<T>& Matrix4<T>::operator*=(const Matrix4<U>& p_other)
{
	return Multiply(p_other);
}


#pragma endregion



template<typename T>
template<typename U>
Vector4<T> Matrix4<T>::Multiply(const Matrix4<U>& p_matrix, const Vector4<T>& p_vector)
{
	Vector4<T> tmpVec = Vector4F::zero;

	tmpVec.x = (p_matrix[0] * p_vector.x)
		+ (p_matrix[1] * p_vector.y)
		+ (p_matrix[2] * p_vector.z)
		+ (p_matrix[3] * p_vector.w);

	tmpVec.y = (p_matrix[4] * p_vector.x)
		+ (p_matrix[5] * p_vector.y)
		+ (p_matrix[6] * p_vector.z)
		+ (p_matrix[7] * p_vector.w);

	tmpVec.z = (p_matrix[8] * p_vector.x)
		+ (p_matrix[9] * p_vector.y)
		+ (p_matrix[10] * p_vector.z)
		+ (p_matrix[11] * p_vector.w);

	tmpVec.w = (p_matrix[12] * p_vector.x)
		+ (p_matrix[13] * p_vector.y)
		+ (p_matrix[14] * p_vector.z)
		+ (p_matrix[15] * p_vector.w);

	return tmpVec;
}

template<typename T>
bool Matrix4<T>::Equals(const Matrix4<T>& p_other) const
{
	for (uint64_t i = 0u; i < 16u; i++)
	{
		if (m_data[i] != p_other.m_data[i])
			return false;
	}
	return true;
}

template<typename T>
template<typename U>
void Matrix4<T>::Set(Matrix4<T>& p_matrix, const Matrix4<U>& p_other)
{
	memcpy_s(p_matrix.m_data, 16 * sizeof(T), p_other.m_data, sizeof(U) * 16);
}

#pragma endregion 

#pragma region Operators
//operators


template<typename T>
Vector4<T> Matrix4<T>::operator*(const Vector4<T>& p_vector)
{
	return Matrix4<T>(*this).Multiply(p_vector);
}

template<typename T>
bool Matrix4<T>::operator==(const Matrix4<T>& p_matrix) const
{
	return Equals(p_matrix);
}

template<typename T>
bool Matrix4<T>::operator!=(const Matrix4<T>& p_matrix) const
{
	return !Equals(p_matrix);
}

template<typename T>
Matrix4<T>& Matrix4<T>::operator=(const Matrix4<T>& p_matrix)
{
	if (this == &p_matrix)
		return *this;
	
	Set(*this, p_matrix);

	return (*this);
}

template<typename T>
Matrix4<T>& Matrix4<T>::operator=(Matrix4<T>&& p_matrix) noexcept
{
	Set(*this, p_matrix);

	return (*this);
}

template<typename T>
template<typename U>
Matrix4<T>& Matrix4<T>::operator=(const Matrix4<U>& p_matrix)
{
	if (this == &p_matrix)
		return *this;
	
	Set(*this, p_matrix);

	return { (*this) };
}

template <typename T>
constexpr std::ostream& GPM::operator<<(std::ostream& p_os, const Matrix4<T>& p_matrix)
{
	p_os << '[' << p_matrix.m_data[0] << "  " << p_matrix.m_data[1] << "  " << p_matrix.m_data[2] << "  " << p_matrix.m_data[3] << "]\n"
		<< '[' << p_matrix.m_data[4] << "  " << p_matrix.m_data[5] << "  " << p_matrix.m_data[6] << "  " << p_matrix.m_data[7] << "]\n"
		<< '[' << p_matrix.m_data[8] << "  " << p_matrix.m_data[9] << "  " << p_matrix.m_data[10] << "  " << p_matrix.m_data[11] << "]\n"
		<< '[' << p_matrix.m_data[12] << "  " << p_matrix.m_data[13] << "  " << p_matrix.m_data[14] << "  " << p_matrix.m_data[15] << "]\n";
	return p_os;
}

template<typename T>
T& Matrix4<T>::operator[](const int p_index)
{
	if (p_index < 0 || p_index > 15)
		throw std::out_of_range("Out of range index in Matrix4");

	return m_data[p_index];
}

template <typename T>
T& Matrix4<T>::operator()(const int p_row, const int p_col)
{
	if (p_row < 0 || p_row > 3)
		throw std::out_of_range("Out of range 'row' in Matrix4");
	else if (p_col < 0 || p_col > 3)
		throw std::out_of_range("Out of range 'col' in Matrix4");


	return m_data[p_row * 4 + p_col];
}

#pragma endregion
