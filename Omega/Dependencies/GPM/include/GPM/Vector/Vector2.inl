#pragma once

#include <GPM/Tools/Utils.h>
#include <stdexcept>
#include <iostream>

template<typename T>
inline constexpr GPM::Vector2<T>::Vector2(const T p_x, const T p_y) : x{ p_x }, y{ p_y } {}

template<typename T>
inline constexpr GPM::Vector2<T>::Vector2(const Vector2<T>& p_other) : x{ p_other.x }, y{ p_other.y } {}

template<typename T>
inline constexpr GPM::Vector2<T>::Vector2(Vector2<T>&& p_other) noexcept
{
	*this = p_other;
}

template<typename T>
inline constexpr void GPM::Vector2<T>::Set(T p_x, T p_y)
{
	x = p_x;
	y = p_y;
}

template <typename T>
const GPM::Vector2<T> GPM::Vector2<T>::zero = Vector2<T>{ 0, 0 };
template <typename T>
const GPM::Vector2<T> GPM::Vector2<T>::up = Vector2<T>{ 0, 1 };
template <typename T>
const GPM::Vector2<T> GPM::Vector2<T>::right = Vector2<T>{ 1, 0 };

#pragma region Member Operator Overloads

template<typename T>
inline constexpr bool GPM::Vector2<T>::operator==(const Vector2<T>& p_other) const
{
	if (x == p_other.x && y == p_other.y)
		return true;

	return false;
}

template<typename T>
inline constexpr bool GPM::Vector2<T>::operator!=(const Vector2<T>& p_other) const
{
	if (x == p_other.x && y == p_other.y)
		return false;

	return true;
}

template<typename T>
inline constexpr GPM::Vector2<T>& GPM::Vector2<T>::operator=(const Vector2<T>& p_other)
{
	x = p_other.x;
	y = p_other.y;

	return *this;
}


template<typename T>
template<typename U>
inline constexpr GPM::Vector2<T>& GPM::Vector2<T>::operator=(const Vector2<U>& p_other)
{
	x = p_other.x;
	y = p_other.y;

	return *this;
}

template<typename T>
inline constexpr GPM::Vector2<T>& GPM::Vector2<T>::operator=(Vector2<T>&& p_other) noexcept
{
	x = p_other.x;
	y = p_other.y;

	return *this;
}

template<typename T>
template<typename U>
inline constexpr GPM::Vector2<T>& GPM::Vector2<T>::operator=(Vector2<U>&& p_other)
{
	x = p_other.x;
	y = p_other.y;

	return *this;
}

#pragma endregion

#pragma region Vector Operations

#pragma region Non-Static

template<typename T>
inline constexpr void GPM::Vector2<T>::Normalize()
{
	const T magnitude = Magnitude();

	if (magnitude == 0)
		*this = zero;

	Divide(magnitude);
}

template<typename T>
inline constexpr GPM::Vector2<T> GPM::Vector2<T>::normalized() const
{
	const T magnitude = Magnitude();

	if (magnitude == 0)
		return zero;

	return { x / magnitude, y / magnitude };
}

template<typename T>
constexpr T GPM::Vector2<T>::Magnitude() const
{
	return std::sqrt(Tools::Utils::Pow(x, 2) + Tools::Utils::Pow(y, 2));
}

#pragma endregion

#pragma region Static
template<typename T>
constexpr GPM::Vector2<T> GPM::Vector2<T>::normalized(const Vector2<T>& p_vector2)
{
	const T magnitude = p_vector2.Magnitude();
	return Vector2<T>{ p_vector2.x / magnitude, p_vector2.y / magnitude };
}

template<typename T>
inline constexpr void GPM::Vector2<T>::Normalize(Vector2<T>& p_vector2)
{
	const T magnitude = p_vector2.Magnitude();

	if (magnitude == 0)
		throw std::logic_error("Vector2::Normalize(Vector2<T>& p_vector2) got a vector2 magnitude of 0");

	p_vector2.x /= magnitude;
	p_vector2.y /= magnitude;
}

template<typename T>
template<typename U>
inline constexpr T GPM::Vector2<T>::Dot(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right)
{
	return (p_vector2Left.x * p_vector2Right.x) + (p_vector2Left.y * p_vector2Right.y);
}

template<typename T>
template<typename U>
inline constexpr T GPM::Vector2<T>::Angle(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right)
{
	const T magnitudeLeft = p_vector2Left.Magnitude();
	const T magnitudeRight = static_cast<T>(p_vector2Right.Magnitude());

	if (magnitudeLeft == 0 || magnitudeRight == 0)
	{
		std::cerr << "Vector2::Angle(const Vector2<T>& p_vector2Left, const Vector2<T>& p_vector2Right) got a vector2 magnitude of 0";
		return static_cast<T>(0);
	}

	T dot = static_cast<T>(p_vector2Left.Dot(p_vector2Right));
	T fraction = dot / (magnitudeLeft * magnitudeRight);

	if (fraction > -1 && fraction < 1)
		return GPM::Tools::Utils::Arccos(dot / (magnitudeLeft * magnitudeRight));

	return 0;
}

template<typename T>
template<typename U>
inline constexpr T GPM::Vector2<T>::Distance(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right)
{
	return std::move(GPM::Tools::Utils::SquareRoot(GPM::Tools::Utils::Pow(p_vector2Left.x - p_vector2Right.x, 2) + GPM::Tools::Utils::Pow(p_vector2Left.y - p_vector2Right.y, 2)));
}

template<typename T>
template<typename U>
inline constexpr GPM::Vector2<T> GPM::Vector2<T>::Lerp(const Vector2<T>& p_vector2Start, const Vector2<U>& p_vector2End, const float p_alpha)
{
	if (p_alpha >= 0 && p_alpha <= 1)
	{
		return { p_vector2Start + (p_vector2End - p_vector2Start) * p_alpha };
	}

	if (p_alpha > 1)
		return p_vector2End;

	return { p_vector2Start };
}

template<typename T>
template<typename U>
inline constexpr GPM::Vector2<T> GPM::Vector2<T>::Perpendicular(const Vector2<U>& p_vector2)
{
	return { p_vector2.y, -p_vector2.x };
}

template<typename T>
template<typename U>
inline constexpr T GPM::Vector2<T>::Dot(const Vector2<U>& p_other) const
{
	return ((x * p_other.x) + (y * p_other.y));
}

template<typename T>
template<typename U>
inline constexpr T GPM::Vector2<T>::Distance(const Vector2<U>& p_other) const
{
	T deltaX = x - p_other.x;
	T deltaY = y - p_other.y;

	return GPM::Tools::Utils::SquareRoot(GPM::Tools::Utils::Pow(deltaX, 2) + GPM::Tools::Utils::Pow(deltaY, 2));
}

template<typename T>
inline constexpr GPM::Vector2<T> GPM::Vector2<T>::Perpendicular() const
{
	return { y, -x };
}

template<typename T>
inline constexpr void GPM::Vector2<T>::Scale(T p_scalar)
{
	Multiply(3);
}

#pragma endregion
#pragma endregion

#pragma region Non-Member Operator Overloads
template <typename T, typename U>
constexpr void GPM::operator-=(GPM::Vector2<T>& p_vector2Left, GPM::Vector2<U> const& p_vector2Right)
{
	p_vector2Left.x -= p_vector2Right.x;
	p_vector2Left.y -= p_vector2Right.y;
}

template<typename T, typename U>
constexpr void GPM::operator*=(Vector2<T>& p_vector2Left, const U& p_scalar)
{
	p_vector2Left.x *= p_scalar;
	p_vector2Left.y *= p_scalar;
}

template<typename T, typename U>
constexpr void GPM::operator/=(Vector2<T>& p_vector2Left, const U& p_scalar)
{
	if (p_scalar == 0)
		throw std::logic_error("Vector2::operator/= attempted division by zero");

	p_vector2Left.x /= p_scalar;
	p_vector2Left.y /= p_scalar;
}


template<typename T>
constexpr std::string GPM::Vector2<T>::ToString() const
{
	std::stringstream stringStream;
	stringStream << "( " << x << ", " << y << " )";
	return { stringStream.str() };
}

template <typename T>
constexpr T GPM::Vector2<T>::operator[](const int p_index) const
{
	if (p_index < 0 || p_index > 1)
		throw std::out_of_range("Out of range access with index:" + std::to_string(p_index) + " in Vector3");

	switch (p_index)
	{
	case 0: return x;
	case 1: return y;
	default: return static_cast<T>(1.0);
	}
}

template<typename T>
constexpr std::ostream& GPM::operator<<(std::ostream& p_stream, const Vector2<T>& p_vector)
{
	p_stream << "( " << p_vector.x << ", " << p_vector.y << " )";
	return  { p_stream };
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator+(Vector2<T> const& p_vector2Left, Vector2<U> const& p_vector2Right)
{
	return GPM::Vector2<T>{p_vector2Left.x + static_cast<T>(p_vector2Right.x), p_vector2Left.y + static_cast<T>(p_vector2Right.y)};
}

template<typename T>
constexpr GPM::Vector2<T> GPM::operator+(Vector2<T> const& p_vector2Left, Vector2<T> const& p_vector2Right)
{
	return GPM::Vector2<T>{p_vector2Left.x + p_vector2Right.x, p_vector2Left.y + p_vector2Right.y};
}

template <typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator+(Vector2<T> const& p_vector2, U const& p_scalar)
{
	return GPM::Vector2<T>{p_vector2.x + static_cast<T>(p_scalar), p_vector2.y + static_cast<T>(p_scalar)};
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator+(U const& p_scalar, Vector2<T> const& p_vector2)
{
	return GPM::Vector2<T>{p_vector2.x + static_cast<T>(p_scalar), p_vector2.y + static_cast<T>(p_scalar)};
}

template<typename T, typename U>
constexpr void GPM::operator+=(Vector2<T>& p_vector2Left, Vector2<U> const& p_vector2Right)
{
	p_vector2Left.x += static_cast<T>(p_vector2Right.x);
	p_vector2Left.y += static_cast<T>(p_vector2Right.y);
}

template<typename T>
constexpr GPM::Vector2<T> GPM::operator-(Vector2<T> const& p_vector2Left, Vector2<T> const& p_vector2Right)
{
	return GPM::Vector2<T>::Subtract(p_vector2Left, p_vector2Right);
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator-(Vector2<T> const& p_vector2Left, Vector2<U> const& p_vector2Right)
{
	return GPM::Vector2<T>::Subtract(p_vector2Left, p_vector2Right);
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator-(Vector2<T> const& p_vector2, U const& p_scalar)
{
	return GPM::Vector2<T>{p_vector2.x - static_cast<T>(p_scalar), p_vector2.y - static_cast<T>(p_scalar)};
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator-(U const& p_scalar, Vector2<T> const& p_vector2)
{
	return GPM::Vector2<T>{p_vector2.x - static_cast<T>(p_scalar), p_vector2.y - static_cast<T>(p_scalar)};
}

template<typename T, typename U>
constexpr GPM::Vector2<U> GPM::operator*(T const& p_scalar, Vector2<U> const& p_vector2)
{
	return Vector2<T>::Multiply(p_vector2, p_scalar);
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator*(Vector2<T> const& p_vector2, U const& p_scalar)
{
	return GPM::Vector2<T>::Multiply(p_vector2, static_cast<T>(p_scalar));
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator*(const Vector2<T>& p_vector2Left, const Vector2<U>& p_vector2Right)
{
	return { p_vector2Left.x * p_vector2Right.x, p_vector2Left.y * p_vector2Right.y };
}

template<typename T, typename U>
constexpr GPM::Vector2<T> GPM::operator/(Vector2<T> const& p_vector2, const U& p_scalar)
{
	if (p_scalar == 0)
		throw std::logic_error("Vector2::operator/ attempted division by zero");

	return GPM::Vector2<T>::Divide(p_vector2, p_scalar);
}

#pragma region Arithmetic Operations
#pragma region Non-Static

template<typename T>
inline constexpr void GPM::Vector2<T>::Add(const GPM::Vector2<T>& p_otherVector2)
{
	x += p_otherVector2.x;
	y += p_otherVector2.y;
}
template<typename T>
inline constexpr void GPM::Vector2<T>::Subtract(const GPM::Vector2<T>& p_otherVector2)
{
	*this -= p_otherVector2;
}
template<typename T>
inline constexpr void GPM::Vector2<T>::Divide(const T& p_scalar)
{
	if (p_scalar == 0)
		throw std::logic_error("Vector2::Divide(const T& p_scalar) attempted division by zero");

	x /= p_scalar;
	y /= p_scalar;
}
template<typename T>
inline constexpr void GPM::Vector2<T>::Multiply(const T& p_scalar)
{
	x *= p_scalar;
	y *= p_scalar;
}
template<typename T>
template<typename U>
inline constexpr bool GPM::Vector2<T>::Equals(const GPM::Vector2<U>& p_otherVector2) const
{
	if (typeid(*this) != typeid(p_otherVector2))
		throw std::logic_error("Vector2::Equals(const GPM::Vector2<U>& p_otherVector2) const attempted to compare 2 Vector2s of different types");

	if (*this == p_otherVector2)
		return true;

	return false;
}
template<typename T>
template<typename U>
constexpr GPM::Vector2<T> GPM::Vector2<T>::Add(const GPM::Vector2<T>& p_vector2Left, const GPM::Vector2<U>& p_vector2Right)
{
	return GPM::Vector2<T>{static_cast<T>(p_vector2Right.x) + p_vector2Left.x, static_cast<T>(p_vector2Right.y) + p_vector2Left.y};
}

#pragma endregion
#pragma region Static

template<typename T>
inline constexpr GPM::Vector2<T> GPM::Vector2<T>::Add(const GPM::Vector2<T>& p_vector2, const T& p_scalar)
{
	return GPM::Vector2<T>{ p_vector2.x + p_scalar, p_vector2.y + p_scalar };
}

template<typename T>
template<typename U>
constexpr GPM::Vector2<T> GPM::Vector2<T>::Subtract(const GPM::Vector2<T>& p_vector2Left, const GPM::Vector2<U>& p_vector2Right)
{
	return GPM::Vector2<T>{static_cast<T>(p_vector2Right.x) - p_vector2Left.x, static_cast<T>(p_vector2Right.y) - p_vector2Left.y};
}

template<typename T>
inline constexpr GPM::Vector2<T> GPM::Vector2<T>::Subtract(const GPM::Vector2<T>& p_vector2, const T& p_scalar)
{
	return GPM::Vector2<T>{ p_vector2.x - p_scalar, p_vector2.y - p_scalar };
}

template<typename T>
constexpr GPM::Vector2<T> GPM::Vector2<T>::Multiply(const GPM::Vector2<T>& p_vector2, const T& p_scalar)
{
	return Vector2<T>({ p_vector2.x * p_scalar, p_vector2.y * p_scalar });
}

template<typename T>
constexpr GPM::Vector2<T> GPM::Vector2<T>::Divide(const GPM::Vector2<T>& p_vector2, const T& p_scalar)
{
	if (p_scalar == 0)
		throw std::logic_error("Vector2::Divide(const GPM::Vector2<T>& p_vector2, const T& p_scalar) attempted division by zero");

	return Vector2<T>({ p_vector2.x / p_scalar, p_vector2.y / p_scalar });
}

#pragma endregion 

#pragma endregion