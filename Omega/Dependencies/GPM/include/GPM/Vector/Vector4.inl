#pragma once

#include <stdexcept>
#include <GPM/Tools/Utils.h>

namespace GPM
{
	template <typename T>
	const Vector4<T> Vector4<T>::zero = Vector4<T>(0, 0, 0, 1);
	template <typename T>
	const Vector4<T> Vector4<T>::one = Vector4<T>(1, 1, 1, 1);
	template <typename T>
	const Vector4<T> Vector4<T>::xAxis = Vector4<T>(1, 0, 0, 1);
	template <typename T>
	const Vector4<T> Vector4<T>::yAxis = Vector4<T>(0, 1, 0, 1);
	template <typename T>
	const Vector4<T> Vector4<T>::zAxis = Vector4<T>(0, 0, 1, 1);

#pragma region Constructors & Assignment

	template <typename T>
	constexpr Vector4<T>::Vector4()
		: x{ 0 }, y{ 0 }, z{ 0 }, w{ 1 } {}

	template <typename T>
	constexpr Vector4<T>::Vector4(T p_x, T p_y, T p_z, T p_w)
		: x{ p_x }, y{ p_y }, z{ p_z }, w{ p_w } {}

	template <typename T>
	constexpr Vector4<T>::Vector4(const Vector3<T>& p_other)
		: x{ p_other.x }, y{ p_other.y }, z{ p_other.z }, w{ 1 } {}

	template <typename T>
	constexpr Vector4<T>::Vector4(Vector3<T>&& p_other)
		: x{ p_other.x }, y{ p_other.y }, z{ p_other.z }, w{ 1 } {}

	template <typename T>
	constexpr Vector4<T>::Vector4(const Vector4<T>& p_other)
	{
		*this = p_other;
	}

	template <typename T>
	constexpr Vector4<T>::Vector4(Vector4<T>&& p_other) noexcept
	{
		*this = p_other;
	}

	template <typename T>
	constexpr Vector4<T>& Vector4<T>::operator=(const Vector4<T>& p_other)
	{
		x = p_other.x;
		y = p_other.y;
		z = p_other.z;
		w = p_other.w;

		return *this;
	}

	template <typename T>
	constexpr Vector4<T>& Vector4<T>::operator=(Vector4<T>&& p_other) noexcept
	{
		x = p_other.x;
		y = p_other.y;
		z = p_other.z;
		w = p_other.w;

		return *this;
	}

#pragma endregion
#pragma region Tests & Comparisons

	template <typename T>
	constexpr bool Vector4<T>::IsParallelTo(const Vector4<T>& p_other) const
	{
		return  { Vector4<T>::Normalize(*this) == Vector4<T>::Normalize(p_other) ||
			Vector4<T>::Normalize(*this) == (Vector4<T>::Normalize(p_other) * -1) };
	}

	template <typename T>
	constexpr bool Vector4<T>::AreParallel(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return  { p_left.IsParallelTo(p_right) };
	}

	template <typename T>
	constexpr bool Vector4<T>::IsPerpendicularTo(const Vector4<T>& p_other) const
	{
		return { Dot(p_other) == 0 };
	}

	template <typename T>
	constexpr bool Vector4<T>::ArePerpendicular(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return  { p_left.IsPerpendicularTo(p_right) };
	}

	template <typename T>
	constexpr bool Vector4<T>::IsHomogenized() const
	{
		return { w == 1 };
	}

	template <typename T>
	constexpr bool Vector4<T>::IsHomogenized(const Vector4<T>& p_vector)
	{
		return { p_vector.w == 1 };
	}

	template <typename T>
	constexpr bool Vector4<T>::IsEqualTo(const Vector4<T>& p_other) const
	{
		return { x == p_other.x && y == p_other.y && z == p_other.z && w == p_other.w };
	}

	template <typename T>
	constexpr bool Vector4<T>::AreEqual(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return { p_left.IsEqualTo(p_right) };
	}

	template <typename T>
	constexpr bool Vector4<T>::operator==(const Vector4<T>& p_other) const
	{
		return  { IsEqualTo(p_other) };
	}

#pragma endregion
#pragma region Arithmetic Operations

#pragma region Add

	template <typename T>
	Vector4<T>& Vector4<T>::Add(const T p_scalar)
	{
		x += p_scalar;
		y += p_scalar;
		z += p_scalar;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Add(const U p_scalar)
	{
		x += p_scalar;
		y += p_scalar;
		z += p_scalar;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Add(const Vector4<T>& p_left, const T p_scalar)
	{
		return { Vector4<T>(p_left).Add(p_scalar) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::Add(const Vector4<T>& p_left, const U p_scalar)
	{
		return { Vector4<T>(p_left).Add(p_scalar) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::Add(const Vector4<T>& p_other)
	{
		x += p_other.x;
		y += p_other.y;
		z += p_other.z;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Add(const Vector4<U>& p_other)
	{
		x += p_other.x;
		y += p_other.y;
		z += p_other.z;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Add(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return { Vector4<T>(p_left).Add(p_right) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::Add(const Vector4<T>& p_left, const Vector4<U>& p_right)
	{
		return { Vector4<T>(p_left).Add(p_right) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator+(const T p_scalar) const
	{
		return { Vector4<T>(*this).Add(p_scalar) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator+=(const T p_scalar)
	{
		return { Add(p_scalar) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator+(const Vector4<T>& p_other) const
	{
		return { Vector4<T>(*this).Add(p_other) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::operator+(const Vector4<U>& p_other) const
	{
		return { Vector4<T>(*this).Add(p_other) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator+=(const Vector4<T>& p_other)
	{
		return { Add(p_other) };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::operator+=(const Vector4<U>& p_other)
	{
		return { Add(p_other) };
	}

#pragma endregion
#pragma region Subtract

	template <typename T>
	Vector4<T>& Vector4<T>::Subtract(const T p_scalar)
	{
		x -= p_scalar;
		y -= p_scalar;
		z -= p_scalar;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Subtract(const U p_scalar)
	{
		x -= p_scalar;
		y -= p_scalar;
		z -= p_scalar;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Subtract(const Vector4<T>& p_left, const T p_scalar)
	{
		return { Vector4<T>(p_left).Subtract(p_scalar) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::Subtract(const Vector4<T>& p_left, const U p_scalar)
	{
		return { Vector4<T>(p_left).Subtract(p_scalar) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::Subtract(const Vector4<T>& p_other)
	{
		x -= p_other.x;
		y -= p_other.y;
		z -= p_other.z;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Subtract(const Vector4<U>& p_other)
	{
		x -= p_other.x;
		y -= p_other.y;
		z -= p_other.z;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Subtract(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return { Vector4<T>(p_left).Subtract(p_right) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::Subtract(const Vector4<T>& p_left, const Vector4<U>& p_right)
	{
		return { Vector4<T>(p_left).Subtract(p_right) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator-(const T p_scalar) const
	{
		return { Vector4<T>(*this).Subtract(p_scalar) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator-=(const T p_scalar)
	{
		return { Subtract(p_scalar) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator-(const Vector4<T>& p_other) const
	{
		return { Vector4<T>(*this).Subtract(p_other) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::operator-(const Vector4<U>& p_other) const
	{
		return { Vector4<T>(*this).Subtract(p_other) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator-=(const Vector4<T>& p_other)
	{
		return { Subtract(p_other) };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::operator-=(const Vector4<U>& p_other)
	{
		return { Subtract(p_other) };
	}

#pragma endregion
#pragma region Multiply

	template <typename T>
	Vector4<T>& Vector4<T>::Multiply(const T p_scalar)
	{
		x *= p_scalar;
		y *= p_scalar;
		z *= p_scalar;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Multiply(const U p_scalar)
	{
		x *= p_scalar;
		y *= p_scalar;
		z *= p_scalar;

		return { *this };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::Multiply(const Vector4<T>& p_other)
	{
		x *= p_other.x;
		y *= p_other.y;
		z *= p_other.z;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Multiply(const Vector4<U>& p_other)
	{
		x *= p_other.x;
		y *= p_other.y;
		z *= p_other.z;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Multiply(const Vector4<T>& p_left, T p_scalar)
	{
		return  { Vector4<T>(p_left).Multiply(p_scalar) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::Multiply(const Vector4<T>& p_left, U p_scalar)
	{
		return  { Vector4<T>(p_left).Multiply(p_scalar) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator*(const T p_scalar) const
	{
		return { Vector4<T>(*this).Multiply(p_scalar) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator*=(const T p_scalar)
	{
		return { Multiply(p_scalar) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator*(const Vector4<T>& p_other) const
	{
		return { Vector4<T>(*this).Multiply(p_other) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::operator*(const Vector4<U>& p_other) const
	{
		return { Vector4<T>(*this).Multiply(p_other) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator*=(const Vector4<T>& p_other)
	{
		return { Multiply(p_other) };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::operator*=(const Vector4<U>& p_other)
	{
		return { Multiply(p_other) };
	}

#pragma endregion
#pragma region Divide

	template <typename T>
	Vector4<T>& Vector4<T>::Divide(const T p_scalar)
	{
		if (p_scalar == 0)
			throw std::logic_error("Division by 0");

		x /= p_scalar;
		y /= p_scalar;
		z /= p_scalar;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Divide(const U p_scalar)
	{
		if (p_scalar == 0)
			throw std::logic_error("Division by 0");

		x /= p_scalar;
		y /= p_scalar;
		z /= p_scalar;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Divide(const Vector4<T>& p_left, const T p_scalar)
	{
		return { Vector4<T>(p_left).Divide(p_scalar) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::Divide(const Vector4<T>& p_left, const U p_scalar)
	{
		return { Vector4<T>(p_left).Divide(p_scalar) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::Divide(const Vector4<T>& p_other)
	{
		if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
			throw std::logic_error("Division by 0");

		x /= p_other.x;
		y /= p_other.y;
		z /= p_other.z;

		return { *this };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::Divide(const Vector4<U>& p_other)
	{
		if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
			throw std::logic_error("Division by 0");

		x /= p_other.x;
		y /= p_other.y;
		z /= p_other.z;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator/(const T p_scalar) const
	{
		return { Vector4<T>(*this).Divide(p_scalar) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator/=(const T p_scalar)
	{
		return { Divide(p_scalar) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::operator/(const Vector4<T>& p_other) const
	{
		return { Vector4<T>(*this).Divide(p_other) };
	}

	template <typename T>
	template <typename U>
	constexpr Vector4<T> Vector4<T>::operator/(const Vector4<U>& p_other) const
	{
		return { Vector4<T>(*this).Divide(p_other) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::operator/=(const Vector4<T>& p_other)
	{
		return { Divide(p_other) };
	}

	template <typename T>
	template <typename U>
	Vector4<T>& Vector4<T>::operator/=(const Vector4<U>& p_other)
	{
		return { Divide(p_other) };
	}

#pragma endregion

#pragma endregion
#pragma region Vector Operations

	template <typename T>
	constexpr float Vector4<T>::Distance(const Vector4<T>& p_vector) const
	{
		if (w != 0 || p_vector.w != 0)
			throw std::logic_error("Can't Compute Distance, one of the params is a direction: W != 0");

		return Tools::Utils::SquareRootF((x - p_vector.x * x - p_vector.x) + (y - p_vector.y) * (y - p_vector.y) + (z - p_vector.z) * (z - p_vector.z));
	}

	template <typename T>
	constexpr float Vector4<T>::Distance(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		if (p_left.w != 0 || p_right.w != 0)
			throw std::logic_error("Can't Compute Distance, one of the params is a point: W != 0");

		return Tools::Utils::SquareRootF((p_left.x - p_right.x * p_left.x - p_right.x) + (p_left.y - p_right.y) * (p_left.y - p_right.y) + (p_left.z - p_right.z) * (p_left.z - p_right.z));
	}

	template <typename T>
	constexpr Vector4<T>& Vector4<T>::Scale(const T p_scale)
	{
		x *= p_scale;
		y *= p_scale;
		z *= p_scale;

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Scale(const Vector4<T>& p_vector, const T p_scale)
	{
		const Vector4<T> result{ p_vector.x * p_scale , p_vector.y *= p_scale, p_vector.z *= p_scale, p_vector.w };

		return { result };
	}

	template <typename T>
	constexpr T Vector4<T>::Magnitude() const
	{
		return { static_cast<T>(Tools::Utils::SquareRootF(x * x + y * y + z * z)) };
	}

	template <typename T>
	constexpr T Vector4<T>::Magnitude(const Vector4<T>& p_vector)
	{
		return { p_vector.Magnitude() };
	}

	template <typename T>
	constexpr T Vector4<T>::MagnitudeSquare() const
	{
		return { static_cast<T>(x * x + y * y + z * z) };
	}

	template <typename T>
	constexpr T Vector4<T>::MagnitudeSquare(const Vector4<T>& p_vector)
	{
		return { p_vector.MagnitudeSquare() };
	}

	template <typename T>
	constexpr T Vector4<T>::Dot(const Vector4<T>& p_other) const
	{
		const Vector4<T> left = Vector4<T>::Homogenize(*this);
		const Vector4<T> right = Vector4<T>::Homogenize(p_other);

		return { left.x * right.x + left.y * right.y + left.z * right.z };
	}

	template <typename T>
	constexpr T Vector4<T>::Dot(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return { p_left.Dot(p_right) };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Cross(const Vector4<T>& p_other) const
	{
		Vector4<T> result;

		if (w == 0 || p_other.w == 0)
			throw std::logic_error("Can't Compute Cross, one of the params is a point: W = 0");

		const Vector4<T> right = Vector4<T>::Homogenize(*this);
		const Vector4<T> left = Vector4<T>::Homogenize(p_other);

		result.x = right.y * left.z - right.z * left.y;
		result.y = right.z * left.x - right.x * left.z;
		result.z = right.x * left.y - right.y * left.x;

		return { result };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Cross(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return p_left.Cross(p_right);
	}

	template <typename T>
	constexpr T Vector4<T>::TripleProduct(const Vector4<T>& p_left, const Vector4<T>& p_middle,
		const Vector4<T>& p_right)
	{
		return p_middle * (Vector4<T>::Dot(p_left, p_right)) - p_right * (Vector4<T>::Dot(p_left, p_middle));
	}

	template <typename T>
	constexpr T Vector4<T>::Angle(const Vector4<T>& p_other) const
	{
		if (w == 0 || p_other.w == 0)
			throw std::logic_error("Can't Compute angle, one of the params is a point: W = 0");

		const float dotProduct = Dot(p_other);
		const T lengthProduct = Magnitude() * p_other.Magnitude();

		const T fractionResult = dotProduct / lengthProduct;

		if (fractionResult > -1 && fractionResult < 1)
			return { static_cast<T>(Tools::Utils::Arccos(fractionResult)) };

		return 0;
	}

	template <typename T>
	constexpr T Vector4<T>::Angle(const Vector4<T>& p_left, const Vector4<T>& p_right)
	{
		return  { p_left.Angle(p_right) };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::Normalize()
	{
		const float length = Magnitude();

		if (length > T(0.0))
		{
			x /= length;
			y /= length;
			z /= length;
		}

		return { *this };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Normalize(const Vector4<T>& p_vector)
	{
		return { Vector4<T>(p_vector).Normalize() };
	}

	template <typename T>
	Vector4<T>& Vector4<T>::Homogenize()
	{
		if (w == 0)
			throw std::logic_error("Can't Homogenize a point: W = 0");

		if (w != 1)
		{
			x /= w;
			y /= w;
			z /= w;
			w = 1;
		}

		return *this;
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Homogenize(const Vector4<T>& p_vector)
	{
		return Vector4<T>(p_vector).Homogenize();
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Lerp(const Vector4<T>& p_start, const Vector4<T>& p_end,
		const float p_interpolationCoefficient)
	{
		if (p_interpolationCoefficient >= 0.0f && p_interpolationCoefficient <= 1.0f)
			return { p_start + (p_end - p_start) * p_interpolationCoefficient };

		if (p_interpolationCoefficient < 0.0f)
			return { p_start };

		return { p_end };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Slerp(const Vector4<T>& p_start, const Vector4<T>& p_end,
		const float p_interpolationCoefficient)
	{
		if (p_interpolationCoefficient >= 0.0f && p_interpolationCoefficient <= 1.0f)
		{
			const float angle = p_start.Angle(p_end) * p_interpolationCoefficient;
			Vector4<T> relativeVector = p_end - p_start * p_start.Dot(p_end);
			relativeVector.Normalize();

			return { (p_start * Tools::Utils::CosF(Tools::Utils::ToRadians(angle))) + (relativeVector * Tools::Utils::SinF(Tools::Utils::ToRadians(angle))) };
		}

		if (p_interpolationCoefficient < 0.0f)
			return { p_start };

		return { p_end };
	}

	template <typename T>
	constexpr Vector4<T> Vector4<T>::Nlerp(const Vector4<T>& p_start,
		const Vector4<T>& p_end, const float p_interpolationCoefficient)
	{
		return Lerp(p_start, p_end, p_interpolationCoefficient).Normalize();
	}

	template <typename T>
	constexpr T Vector4<T>::operator[](const int p_index) const
	{
		if (p_index < 0 || p_index > 3)
			throw std::out_of_range("Out of range access with index:" + std::to_string(p_index) + " in Vector4");

		switch (p_index)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		case 3: return w;
		default: return static_cast<T>(1.0);
		}
	}


#pragma endregion
#pragma region Conversions

	template <typename T>
	constexpr std::string Vector4<T>::ToString() const
	{
		return { std::string("x : " + std::to_string(x) + " y : " + std::to_string(y) +
			" z : " + std::to_string(z) + " w : " + std::to_string(w)) };
	}

	template <typename T>
	constexpr std::string Vector4<T>::ToString(const Vector4<T>& p_vector)
	{
		return { p_vector.ToString() };
	}

#pragma endregion

#pragma region Outside Operators

	template <typename T>
	constexpr std::ostream& operator<<(std::ostream& p_stream, const Vector4<T>& p_vector)
	{
		p_stream << "x : " << p_vector.x << " y : " << p_vector.y <<
			" z : " << p_vector.z << " w : " << p_vector.w;
		return  { p_stream };
	}

	template <typename T>
	constexpr Vector4<T> operator+(const T p_scalar, const Vector4<T>& p_vector)
	{
		return { p_vector + p_scalar };
	}

	template <typename T, typename U>
	constexpr Vector4<T> operator+(const U p_scalar, const Vector4<T>& p_vector)
	{
		return { p_vector + p_scalar };
	}

	template <typename T>
	constexpr Vector4<T>& operator+=(const T p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector += p_scalar };
	}

	template <typename T, typename U>
	constexpr Vector4<T>& operator+=(const U p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector += p_scalar };
	}

	template <typename T>
	constexpr Vector4<T> operator-(const T p_scalar, const Vector4<T>& p_vector)
	{
		return { p_vector - p_scalar };
	}

	template <typename T, typename U>
	constexpr Vector4<T> operator-(const U p_scalar, const Vector4<T>& p_vector)
	{
		return { p_vector - p_scalar };
	}

	template <typename T>
	constexpr Vector4<T>& operator-=(const T p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector -= p_scalar };
	}

	template <typename T, typename U>
	constexpr Vector4<T>& operator-=(const U p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector -= p_scalar };
	}

	template <typename T>
	constexpr Vector4<T> operator*(const T p_scalar, const Vector4<T>& p_vector)
	{
		return { p_vector * p_scalar };
	}

	template <typename T, typename U>
	constexpr Vector4<T> operator*(const U p_scalar, const Vector4<T>& p_vector)
	{
		return { p_vector * p_scalar };
	}

	template <typename T>
	constexpr Vector4<T>& operator*=(const T p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector *= p_scalar };
	}

	template <typename T, typename U>
	constexpr Vector4<T>& operator*=(const U p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector *= p_scalar };
	}

	template <typename T>
	constexpr Vector4<T> operator/(const T p_scalar, const Vector4<T>& p_vector)
	{
		return p_vector / p_scalar;
	}

	template <typename T, typename U>
	constexpr Vector4<T> operator/(const U p_scalar, const Vector4<T>& p_vector)
	{
		return p_vector / p_scalar;
	}

	template <typename T>
	constexpr Vector4<T>& operator/=(const T p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector /= p_scalar };
	}

	template <typename T, typename U>
	constexpr Vector4<T>& operator/=(const U p_scalar, Vector4<T>& p_vector)
	{
		return { p_vector /= p_scalar };
	}

#pragma endregion
}
