#pragma once

#include <utility>
#include <stdexcept>
#include <GPM/Tools/Utils.h>

// const Quaternion Quaternion::identity = Quaternion{ 0.0, 0.0, 0.0, 1.0 };

namespace GPM
{
#pragma region Constructors & Assignment
	inline Quaternion::Quaternion()
		: w{ 1.0 }, x{ 0.0 }, y{ 0.0 }, z{ 0.0 }
	{
	}

	inline Quaternion::Quaternion(const double p_x, const double p_y, const double p_z,
		const double p_w)
		: w{ p_w }, x{ p_x }, y{ p_y }, z{ p_z }
	{
	}

	inline Quaternion::Quaternion(const double p_scalar, const Vector3<double>& p_vector)
		: w{ p_scalar }, x{ p_vector.x }, y{ p_vector.y }, z{ p_vector.z }
	{
	}

	inline Quaternion::Quaternion(const Quaternion& p_other)
		: w{ p_other.w }, x{ p_other.x }, y{ p_other.y }, z{ p_other.z }
	{
	}

	inline Quaternion::Quaternion(Quaternion&& p_other) noexcept
		: w{ p_other.w }, x{ p_other.x }, y{ p_other.y }, z{ p_other.z }
	{
	}

	inline Quaternion::Quaternion(const Matrix3<double>& p_matrix)
		: w{ 1.0 }, x{ 0.0 }, y{ 0.0 }, z{ 0.0 }
	{
		const double trace = p_matrix.m_data[0] + p_matrix.m_data[4] + p_matrix.m_data[8];

		if (trace > 0.0f)
		{
			//s=4*qw

			w = 0.5 * Tools::Utils::SquareRoot(1.0 + trace);
			const double S = 0.25 / w;

			x = S * (p_matrix.m_data[5] - p_matrix.m_data[7]);
			y = S * (p_matrix.m_data[6] - p_matrix.m_data[2]);
			z = S * (p_matrix.m_data[1] - p_matrix.m_data[3]);
		}
		else if (p_matrix.m_data[0] > p_matrix.m_data[4] && p_matrix.m_data[0] > p_matrix.m_data[8])
		{
			//s=4*qx

			x = 0.5 * Tools::Utils::
				SquareRoot(1.0 + p_matrix.m_data[0] - p_matrix.m_data[4] - p_matrix.m_data[8]);
			const double X = 0.25 / x;

			y = X * (p_matrix.m_data[3] + p_matrix.m_data[1]);
			z = X * (p_matrix.m_data[6] + p_matrix.m_data[2]);
			w = X * (p_matrix.m_data[5] - p_matrix.m_data[7]);
		}
		else if (p_matrix.m_data[4] > p_matrix.m_data[8])
		{
			//s=4*qy

			y = 0.5 * Tools::Utils::
				SquareRoot(1.0 - p_matrix.m_data[0] + p_matrix.m_data[4] - p_matrix.m_data[8]);
			const double Y = 0.25 / y;
			x = Y * (p_matrix.m_data[3] + p_matrix.m_data[1]);
			z = Y * (p_matrix.m_data[7] + p_matrix.m_data[5]);
			w = Y * (p_matrix.m_data[6] - p_matrix.m_data[2]);
		}
		else
		{
			//s=4*qz

			z = 0.5 * Tools::Utils::
				SquareRoot(1.0 - p_matrix.m_data[0] - p_matrix.m_data[4] + p_matrix.m_data[8]);
			const double Z = 0.25 / z;
			x = Z * (p_matrix.m_data[6] + p_matrix.m_data[2]);
			y = Z * (p_matrix.m_data[7] + p_matrix.m_data[5]);
			w = Z * (p_matrix.m_data[1] - p_matrix.m_data[3]);
		}
	}

	inline Quaternion::Quaternion(const Matrix4<double>& p_matrix)
		: w{ 1.0 }, x{ 0.0 }, y{ 0.0 }, z{ 0.0 }
	{
		w = Tools::Utils::SquareRoot(std::max(0.0, 1.0 + p_matrix.m_data[0] + p_matrix.m_data[5] + p_matrix.m_data[10]))
			/ 2.0;
		x = Tools::Utils::SquareRoot(std::max(0.0, 1.0 + p_matrix.m_data[0] - p_matrix.m_data[5] - p_matrix.m_data[10]))
			/ 2.0;
		y = Tools::Utils::SquareRoot(std::max(0.0, 1.0 - p_matrix.m_data[0] + p_matrix.m_data[5] - p_matrix.m_data[10]))
			/ 2.0;
		z = Tools::Utils::SquareRoot(std::max(0.0, 1.0 - p_matrix.m_data[0] - p_matrix.m_data[5] + p_matrix.m_data[10]))
			/ 2.0;

		x *= Tools::Utils::Sign(x * (p_matrix.m_data[9] - p_matrix.m_data[6]));
		y *= Tools::Utils::Sign(y * (p_matrix.m_data[2] - p_matrix.m_data[8]));
		z *= Tools::Utils::Sign(z * (p_matrix.m_data[4] - p_matrix.m_data[1]));
	}

	inline Quaternion::Quaternion(const Matrix4<float>& p_matrix)
		: w{ 1.0 }, x{ 0.0 }, y{ 0.0 }, z{ 0.0 }
	{
		w = Tools::Utils::SquareRoot(std::max(0.0, 1.0 + p_matrix.m_data[0] + p_matrix.m_data[5] + p_matrix.m_data[10]))
			/ 2.0;
		x = Tools::Utils::SquareRoot(std::max(0.0, 1.0 + p_matrix.m_data[0] - p_matrix.m_data[5] - p_matrix.m_data[10]))
			/ 2.0;
		y = Tools::Utils::SquareRoot(std::max(0.0, 1.0 - p_matrix.m_data[0] + p_matrix.m_data[5] - p_matrix.m_data[10]))
			/ 2.0;
		z = Tools::Utils::SquareRoot(std::max(0.0, 1.0 - p_matrix.m_data[0] - p_matrix.m_data[5] + p_matrix.m_data[10]))
			/ 2.0;

		x *= Tools::Utils::Sign(x * (p_matrix.m_data[9] - p_matrix.m_data[6]));
		y *= Tools::Utils::Sign(y * (p_matrix.m_data[2] - p_matrix.m_data[8]));
		z *= Tools::Utils::Sign(z * (p_matrix.m_data[4] - p_matrix.m_data[1]));
	}

	inline Quaternion::Quaternion(const Vector3<double>& p_axis,
		const double           p_angleInRadians)
		: w{ 1.0 }, x{ 0.0 }, y{ 0.0 }, z{ 0.0 }
	{
		const double angleDivided = p_angleInRadians / 2.0;

		w = Tools::Utils::Cos(angleDivided);

		const double sinAngle = Tools::Utils::Sin(angleDivided);

		x = sinAngle * p_axis.x;
		y = sinAngle * p_axis.y;
		z = sinAngle * p_axis.z;
	}

	inline Quaternion Quaternion::MakeFromEuler(const double p_yawAlpha, const double p_roll, const double p_yaw)
	{
		Quaternion rotation;
		rotation.SetFromEuler(p_yawAlpha, p_roll, p_yaw);

		return rotation;
	}

	inline Quaternion Quaternion::MakeFromEuler(const Vector3<double>& p_euler)
	{
		Quaternion rotation;
		rotation.SetFromEuler(p_euler.x, p_euler.y, p_euler.z);

		return rotation;
	}

	inline void Quaternion::SetFromEuler(const Vector3<double>& p_euler) const
	{
		SetFromEuler(Vector3<double>(p_euler.x, p_euler.y, p_euler.z));
	}

	inline void Quaternion::SetFromEuler(const double p_roll, const double p_pith, const double p_yaw)
	{
		const double roll = Tools::Utils::ToRadians(p_roll) * 0.5;
		const double pitch = Tools::Utils::ToRadians(p_pith) * 0.5;
		const double yaw = Tools::Utils::ToRadians(p_yaw) * 0.5;

		const double cy = cos(yaw);
		const double sy = sin(yaw);
		const double cp = cos(pitch);
		const double sp = sin(pitch);
		const double cr = cos(roll);
		const double sr = sin(roll);

		w = cr * cp * cy - sr * sp * sy;
		x = sr * cp * cy + cr * sp * sy;
		y = cr * sp * cy - sr * cp * sy;
		z = cr * cp * sy + sr * sp * cy;
	}

	inline Quaternion& Quaternion::operator=(Quaternion&& p_other) noexcept
	{
		w = p_other.w;
		x = p_other.x;
		y = p_other.y;
		z = p_other.z;

		return *this;
	}

	inline bool Quaternion::IsIdentity() const
	{
		return x == 0.0 && y == 0.0 && z == 0.0;
	}

	inline bool Quaternion::IsPure() const
	{
		return w > -0.000009 && w < 0.000009;
	}

	inline bool Quaternion::IsNormalized() const
	{
		return Norm() > 0.999990 && Norm() < 1.000009;
	}

	inline bool Quaternion::operator==(const Quaternion& p_otherQuaternion) const
	{
		return w == p_otherQuaternion.w && x == p_otherQuaternion.x && y == p_otherQuaternion.y && z ==
			p_otherQuaternion.z;
	}

	inline bool Quaternion::operator!=(const Quaternion& p_otherQuaternion) const
	{
		return w != p_otherQuaternion.w || x != p_otherQuaternion.x || y != p_otherQuaternion.y || z !=
			p_otherQuaternion.z;
	}

	inline Quaternion Quaternion::operator+(const Quaternion& p_otherQuaternion) const
	{
		return {
			Quaternion{
				x + p_otherQuaternion.x, y + p_otherQuaternion.y, z + p_otherQuaternion.z, w + p_otherQuaternion.w
			}
		};
	}

	inline Quaternion& Quaternion::operator+=(const Quaternion& p_otherQuaternion)
	{
		w += p_otherQuaternion.w;
		x += p_otherQuaternion.x;
		y += p_otherQuaternion.y;
		z += p_otherQuaternion.z;

		return { *this };
	}

	inline Quaternion Quaternion::operator-(const Quaternion& p_otherQuaternion) const
	{
		return {
			Quaternion{
				x - p_otherQuaternion.x, y - p_otherQuaternion.y, z - p_otherQuaternion.z, w - p_otherQuaternion.w
			}
		};
	}

	inline Quaternion& Quaternion::operator-=(const Quaternion& p_otherQuaternion)
	{
		w -= p_otherQuaternion.w;
		x -= p_otherQuaternion.x;
		y -= p_otherQuaternion.y;
		z -= p_otherQuaternion.z;

		return { *this };
	}

	inline double Quaternion::DotProduct(const Quaternion& p_otherQuaternion) const
	{
		return w * p_otherQuaternion.w + x * p_otherQuaternion.x + y * p_otherQuaternion.y + z * p_otherQuaternion.z;
	}

	inline double Quaternion::DotProduct(const Quaternion& p_left, const Quaternion& p_right)
	{
		return p_left.w * p_right.w + p_left.x * p_right.x + p_left.y * p_right.y + p_left.z * p_right.z;
	}

	inline Quaternion& Quaternion::operator*=(const double p_scale)
	{
		w *= p_scale;
		x *= p_scale;
		y *= p_scale;
		z *= p_scale;

		return { *this };
	}

	inline Quaternion Quaternion::operator*(const double p_scale) const
	{
		return { Quaternion{x * p_scale, y * p_scale, z * p_scale, w * p_scale} };
	}

	inline Quaternion Quaternion::operator*(const Quaternion& p_otherQuaternion) const
	{
		return { (*this).Multiply(p_otherQuaternion) };
	}

	inline Quaternion& Quaternion::operator*=(const Quaternion& p_otherQuaternion)
	{
		(*this) = Multiply(p_otherQuaternion);

		return { (*this) };
	}

	inline Quaternion Quaternion::operator*(const Vector3<double>& p_toMultiply) const
	{
		const double sPart = -(x * p_toMultiply.x + y * p_toMultiply.y + z * p_toMultiply.z);
		const double xPart = w * p_toMultiply.x + y * p_toMultiply.z - z * p_toMultiply.y;
		const double yPart = w * p_toMultiply.y + z * p_toMultiply.x - x * p_toMultiply.z;
		const double zPart = w * p_toMultiply.z + x * p_toMultiply.y - y * p_toMultiply.x;

		return { Quaternion{sPart, Vector3<double>{xPart, yPart, zPart}} };
	}

	inline Quaternion& Quaternion::operator*=(const Vector3<double>& p_toMultiply)
	{
		const double sPart = -(x * p_toMultiply.x + y * p_toMultiply.y + z * p_toMultiply.z);
		const double xPart = w * p_toMultiply.x + y * p_toMultiply.z - z * p_toMultiply.y;
		const double yPart = w * p_toMultiply.y + z * p_toMultiply.x - x * p_toMultiply.z;
		const double zPart = w * p_toMultiply.z + x * p_toMultiply.y - y * p_toMultiply.x;

		w = sPart;
		x = xPart;
		y = yPart;
		z = zPart;

		return { (*this) };
	}

	inline Vector3<float> Quaternion::operator*(const Vector3<float>& p_toMultiply) const
	{
		const float num = static_cast<float>(x) * 2.0f;
		const float num2 = static_cast<float>(y) * 2.0f;
		const float num3 = static_cast<float>(z) * 2.0f;
		const float num4 = static_cast<float>(x) * num;
		const float num5 = static_cast<float>(y) * num2;
		const float num6 = static_cast<float>(z) * num3;
		const float num7 = static_cast<float>(x) * num2;
		const float num8 = static_cast<float>(x) * num3;
		const float num9 = static_cast<float>(y) * num3;
		const float num10 = static_cast<float>(w) * num;
		const float num11 = static_cast<float>(w) * num2;
		const float num12 = static_cast<float>(w) * num3;
		Vector3F    result;
		result.x = (1.f - (num5 + num6)) * p_toMultiply.x + (num7 - num12) * p_toMultiply.y + (
			num8 + num11) *
			p_toMultiply.z;
		result.y = (num7 + num12) * p_toMultiply.x + (1.f - (num4 + num6)) * p_toMultiply.y + (
			num9 - num10) *
			p_toMultiply.z;
		result.z = (num8 - num11) * p_toMultiply.x + (num9 + num10) * p_toMultiply.y + (
			1.f - (num4 + num5)) *
			p_toMultiply.z;
		return result;
	}

	inline Quaternion Quaternion::Multiply(const Quaternion& p_quaternion) const
	{
		Quaternion result;
		result.x = x * p_quaternion.w + y * p_quaternion.z - z * p_quaternion.y + w *
			p_quaternion.x;
		result.y = -x * p_quaternion.z + y * p_quaternion.w + z * p_quaternion.x + w *
			p_quaternion.y;
		result.z = x * p_quaternion.y - y * p_quaternion.x + z * p_quaternion.w + w *
			p_quaternion.z;
		result.w = -x * p_quaternion.x - y * p_quaternion.y - z * p_quaternion.z + w *
			p_quaternion.w;
		return { result };
	}

	inline double Quaternion::Norm() const
	{
		return Tools::Utils::SquareRoot(w * w + x * x + y * y + z * z);
	}

	inline Quaternion& Quaternion::Inverse()
	{
		double absoluteValue = NormSquare();
		absoluteValue = 1.0 / absoluteValue;

		const Quaternion conjugateValue = Conjugate();

		const double scalar = conjugateValue.w * (absoluteValue);

		w = scalar;
		x = conjugateValue.x * absoluteValue;
		y = conjugateValue.y * absoluteValue;
		z = conjugateValue.z * absoluteValue;

		return { (*this) };
	}

	inline Quaternion Quaternion::Inverse(const Quaternion& p_quaternion)
	{
		double absoluteValue = p_quaternion.NormSquare();
		absoluteValue = 1.0 / absoluteValue;

		const Quaternion conjugateValue = Conjugate(p_quaternion);

		return {
			Quaternion{
				conjugateValue.x * absoluteValue, conjugateValue.y * absoluteValue,
				conjugateValue.z * absoluteValue, conjugateValue.w * absoluteValue
			}
		};
	}

	inline Quaternion& Quaternion::Conjugate()
	{
		x *= -1.0;
		y *= 1.0;
		z *= 1.0;

		return { (*this) };
	}

	inline Quaternion Quaternion::Conjugate(const Quaternion& p_quaternion)
	{
		return {
			Quaternion{
				p_quaternion.x * -1.0, p_quaternion.y * -1.0, p_quaternion.z * -1.0,
				p_quaternion.w
			}
		};
	}

	inline Quaternion& Quaternion::ConvertToUnitNormQuaternion()
	{
		const double angle = Tools::Utils::ToRadians(w);

		Normalize();
		w = Tools::Utils::Cos(angle * 0.5);
		x = x * Tools::Utils::Sin(angle * 0.5);
		y = y * Tools::Utils::Sin(angle * 0.5);
		z = z * Tools::Utils::Sin(angle * 0.5);

		return { (*this) };
	}

	inline Vector3<double> Quaternion::RotateVectorAboutAngleAndAxis(
		const double           p_angle, const Vector3<double>& p_axis,
		const Vector3<double>& p_vectorToRotate)
	{
		const Quaternion p{ 0, p_vectorToRotate };

		//normalize the axis
		const Vector3<double> uAxis = p_axis.Normalized();

		//create the real quaternion
		Quaternion q{ p_angle, uAxis };

		//convert quaternion to unit norm quaternion
		q.ConvertToUnitNormQuaternion();

		const Quaternion qInverse = Inverse(q);

		const Quaternion rotatedVector = q * p * qInverse;

		return Vector3<double>(rotatedVector.x, rotatedVector.y, rotatedVector.z);
	}

	inline double Quaternion::operator[](const int p_index) const
	{
		if (p_index < 0 || p_index > 3)
			throw std::out_of_range("Out of range access with index:" + std::to_string(p_index) +
				" in Quaternion");

		switch (p_index)
		{
		case 0: return w;
		case 1: return x;
		case 2: return y;
		case 3: return z;
		default: return 1.0;
		}
	}

	inline Vector3<double> Quaternion::GetRotationAxis() const
	{
		const double s = sqrt(std::max(1.0 - (w * w), 0.0));

		if (s >= 0.0001)
		{
			return Vector3F(static_cast<float>(x / s), static_cast<float>(y / s),
				static_cast<float>(z / s));
		}

		return Vector3F(1.0f, 0.0f, 0.0f);
	}

	inline double Quaternion::GetXAxisValue() const
	{
		return x;
	}

	inline double Quaternion::GetYAxisValue() const
	{
		return y;
	}

	inline double Quaternion::GetZAxisValue() const
	{
		return z;
	}

	inline double Quaternion::GetRealValue() const
	{
		return w;
	}

	inline void Quaternion::SetXAxisValue(const double p_xValue)
	{
		x = p_xValue;
	}

	inline void Quaternion::SetYAxisValue(const double p_yValue)
	{
		y = p_yValue;
	}

	inline void Quaternion::SetZAxisValue(const double p_zValue)
	{
		z = p_zValue;
	}

	inline void Quaternion::SetRealValue(const double p_realValue)
	{
		w = p_realValue;
	}

	inline Quaternion Quaternion::LookRotation(const Vector3<double>& p_forward,
		const Vector3<double>& p_upwards) const
	{
		const Vector3<double> forwardVector = (p_upwards - p_forward).Normalized();

		const double dot = Vector3<double>::forward.Dot(forwardVector);

		if (Tools::Utils::Abs<double>(dot - (-1.0)) < 0.000001)
		{
			return Quaternion(Vector3<double>::up.x, Vector3<double>::up.y,
				Vector3<double>::up.z, static_cast<double>(Tools::M_PI));
		}
		if (Tools::Utils::Abs<double>(dot - (1.0)) < 0.000001)
		{
			return Quaternion{ 0.0, 0.0, 0.0, 1.0 };
		}

		const double    rotAngle = Tools::Utils::Arccos(dot);
		Vector3<double> rotAxis = Vector3<double>::
			Cross(Vector3<double>::forward, forwardVector);
		rotAxis = rotAxis.Normalized();
		return CreateFromAxisAngle(rotAxis, rotAngle);
	}

	inline Quaternion Quaternion::CreateFromAxisAngle(const Vector3<double>& p_axis,
		const double           p_angle)
	{
		const double halfAngle = p_angle * 0.5;
		const double s = Tools::Utils::Sin(halfAngle);

		Quaternion q;
		q.x = p_axis.x * s;
		q.y = p_axis.y * s;
		q.z = p_axis.z * s;
		q.w = Tools::Utils::Cos(halfAngle);

		return q;
	}

	inline Quaternion Quaternion::Lerp(const Quaternion& p_start, const Quaternion& p_end,
		const double      p_alpha)
	{
		const double coefficient = 1.0 - p_alpha;

		return {
			Quaternion{
				coefficient * p_start.x + p_alpha * p_end.x,
				coefficient * p_start.y + p_alpha * p_end.y,
				coefficient * p_start.z + p_alpha * p_end.z,
				coefficient * p_start.w + p_alpha * p_end.w
			} // .Normalize(); // Cancel the interpolation ?
		};
	}

	inline Quaternion Quaternion::Slerp(const Quaternion& p_start, const Quaternion& p_end,
		const double      p_alpha)
	{
		const Quaternion qStartNormalized = Normalize(p_start);
		const Quaternion qEndNormalized = Normalize(p_end);

		double dot = DotProduct(qStartNormalized, qEndNormalized);

		//clamp values (just in case) because ArcCos only works from -1 to 1
		if (dot > 1.0)
		{
			dot = 1.0;
		}
		else if (dot < -1.0)
			dot = -1.0;

		const double theta = Tools::Utils::Arccos(dot) * p_alpha;
		Quaternion   relativeQuaternion = qEndNormalized - qStartNormalized * dot;
		relativeQuaternion.Normalize();

		Quaternion result =
			qStartNormalized * Tools::Utils::Cos(theta) + relativeQuaternion * Tools::Utils::
			Sin(theta);

		return result;
	}

	inline Quaternion Quaternion::SlerpShortestPath(
		const Quaternion& p_start, const Quaternion& p_end, const double p_alpha)
	{
		Quaternion       qStartNormalized = Normalize(p_start);
		const Quaternion qEndNormalized = Normalize(p_end);

		double dot = DotProduct(qStartNormalized, qEndNormalized);

		// If the dot product is negative,
		// Slerp will not look for the closest rotation -> It will spin the other way around.
		if (dot < 0.0)
		{
			qStartNormalized.w = -qStartNormalized.w;
			qStartNormalized.x = -qStartNormalized.x;
			qStartNormalized.y = -qStartNormalized.y;
			qStartNormalized.z = -qStartNormalized.z;
			dot = -dot;
		}

		//clamp values (just in case) because ArcCos only works from -1 to 1
		if (dot > 1.0)
		{
			dot = 1.0;
		}
		else if (dot < -1.0)
			dot = -1.0;

		const double theta = Tools::Utils::Arccos(dot) * p_alpha;
		Quaternion   relativeQuaternion = qEndNormalized - qStartNormalized * dot;
		relativeQuaternion.Normalize();

		Quaternion result =
			qStartNormalized * Tools::Utils::Cos(theta) + relativeQuaternion * Tools::Utils::
			Sin(theta);

		return result;
	}

	inline Quaternion Quaternion::Nlerp(const Quaternion& p_start,
		const Quaternion& p_end, const double p_alpha)
	{
		return Lerp(p_start, p_end, p_alpha).Normalize();
	}

	inline Vector3<double> Quaternion::RotateRelativeToPivot(
		const Vector3<double>& p_point, const Quaternion& p_quaternion) const
	{
		const Vector3D Q(p_quaternion.x, p_quaternion.y, p_quaternion.z);
		const Vector3D T = Vector3D::Cross(Q, p_point) * 2.0;

		return p_point + (T * p_quaternion.w) + Vector3D::Cross(Q, T);
	}

	inline Vector3<double> Quaternion::RotateRelativeToPivot(
		const Vector3<double>& p_point, const Vector3<double>& p_pivot,
		const Quaternion& p_quaternion)
	{
		const Quaternion rotator;
		const Vector3D   toRotate = p_point - p_pivot;

		return rotator.RotateRelativeToPivot(toRotate, p_quaternion);
	}

	constexpr double Quaternion::NormSquare() const
	{
		return w * w + x * x + y * y + z * z;
	}

	inline double Quaternion::GetAngle() const
	{
		return 2.0 * acos(w);
	}

	inline double Quaternion::GetAngle(const Quaternion& p_target)
	{
		return p_target.GetAngle();
	}

	inline Quaternion& Quaternion::Normalize()
	{
		if (Norm() > 0.0)
		{
			const double normValue = 1.0 / Norm();

			w *= normValue;
			x *= normValue;
			y *= normValue;
			z *= normValue;
		}

		return { (*this) };
	}

	inline Quaternion Quaternion::Normalize(const Quaternion& p_quaternion)
	{
		double scalar = 0.0;

		Vector3<double> vector{};

		if (p_quaternion.Norm() != 0.0)
		{
			const double normValue = 1.0 / p_quaternion.Norm();

			scalar = p_quaternion.w * normValue;
			vector.x = p_quaternion.x * normValue;
			vector.y = p_quaternion.y * normValue;
			vector.z = p_quaternion.z * normValue;
		}

		return { Quaternion{scalar, vector} };
	}

	inline Quaternion Quaternion::ToUnitNormQuaternion()
	{
		const double angle = Tools::Utils::ToRadians(w);

		Normalize();

		return {
			Quaternion{
				x * Tools::Utils::Sin(angle * 0.5), y * Tools::Utils::Sin(angle * 0.5),
				z * Tools::Utils::Sin(angle * 0.5), Tools::Utils::Cos(angle * 0.5)
			}
		};
	}

	inline Vector3<float> Quaternion::ToEuler() const
	{
		// This is a kind of hack because when the input Quaternion is {0.5f, 0.5f, -0.5f, 0.5f} or
		// {0.5f, 0.5f, 0.5f, -0.5f}, the output value is incorrect.
		if (*this == Quaternion{ 0.5, 0.5, -0.5, 0.5 }) return { 90.0f, 90.0f, 0.0f };
		if (*this == Quaternion{ 0.5, 0.5, 0.5, -0.5 }) return { -90.0f, -90.0f, 0.0f };

		// roll (x-axis rotation)
		const float sinr_cosp = static_cast<float>(+2.0 * (w * x + y * z));
		const float cosr_cosp = static_cast<float>(1.0 - 2.0 * (x * x + y * y));
		const float roll = atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float       pitch = 0.0f;
		const float sinp = static_cast<float>(+2.0 * (w * y - z * x));
		if (fabs(sinp) >= 1.0f)
			pitch = static_cast<float>(copysign(Tools::M_PI / 2.0, sinp));
		// use 90 degrees if out of range
		else
			pitch = asin(sinp);

		// yaw (z-axis rotation)
		const float siny_cosp = static_cast<float>(+2.0 * (w * z + x * y));
		const float cosy_cosp = static_cast<float>(+1.0 - 2.0 * (y * y + z * z));
		const float yaw = atan2(siny_cosp, cosy_cosp);

		return (Vector3F(Tools::Utils::ToDegrees(roll), Tools::Utils::ToDegrees(pitch),
			Tools::Utils::ToDegrees(yaw))); // XYZ
	}

	inline Quaternion Quaternion::ToQuaternion(const Vector3<double>& p_euler)
	{
		return { ToQuaternion(p_euler.x, p_euler.y, p_euler.z) };
	}

	inline Quaternion Quaternion::ToQuaternion(const double p_yaw, const double p_pitch,
		const double p_roll)
	{
		Quaternion result;

		const double cosYaw = Tools::Utils::Cos(p_yaw * 0.5);
		const double sinYaw = Tools::Utils::Sin(p_yaw * 0.5);
		const double cosPitch = Tools::Utils::Cos(p_pitch * 0.5);
		const double sinPitch = Tools::Utils::Sin(p_pitch * 0.5);
		const double cosRoll = Tools::Utils::Cos(p_roll * 0.5);
		const double sinRoll = Tools::Utils::Sin(p_roll * 0.5);

		result.w = cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll;
		result.x = cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll;
		result.y = sinYaw * cosPitch * sinRoll + cosYaw * sinPitch * cosRoll;
		result.z = sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll;

		return { result };
	}

	inline std::string Quaternion::ToString() const
	{
		return {
			std::string("(w: " + std::to_string(w) + "; x: " + std::to_string(x) + ", y: " + std
						::to_string(y) +
						", z: " + std::to_string(z)) + ')'
		};
	}

	inline std::string Quaternion::ToString(const Quaternion& p_quaternion)
	{
		return { p_quaternion.ToString() };
	}

	inline Matrix3<float> Quaternion::ToMatrix3() const
	{
		Matrix3<float> result;

		const float    fw = static_cast<float>(w);
		Vector3<float> faxis{};
		faxis.x = static_cast<float>(x);
		faxis.y = static_cast<float>(y);
		faxis.z = static_cast<float>(z);

		result.m_data[0] = 2.0f * (fw * fw + faxis.x * faxis.x) - 1.0f;
		result.m_data[3] = 2.0f * (faxis.x * faxis.y - fw * faxis.z);
		result.m_data[6] = 2.0f * (faxis.x * faxis.z + fw * faxis.y);

		result.m_data[1] = 2.0f * (faxis.x * faxis.y + fw * faxis.z);
		result.m_data[4] = 2.0f * (fw * fw + faxis.y * faxis.y) - 1.0f;
		result.m_data[7] = 2.0f * (faxis.y * faxis.z - fw * faxis.x);

		result.m_data[2] = 2.0f * (faxis.x * faxis.z - fw * faxis.y);
		result.m_data[5] = 2.0f * (faxis.y * faxis.z + fw * faxis.x);
		result.m_data[8] = 2.0f * (fw * fw + faxis.z * faxis.z) - 1.0f;

		return result;
	}

	inline Matrix4<float> Quaternion::ToMatrix4() const
	{
		if (!IsNormalized())
			throw std::logic_error("Cannot convert non-normalized quaternions to Matrix4");

		Matrix4<float> result{};
		const float    sqw = static_cast<float>(w * w);
		const float    sqx = static_cast<float>(x * x);
		const float    sqy = static_cast<float>(y * y);
		const float    sqz = static_cast<float>(z * z);

		// invs (inverse square length) is only required if quaternion is not already normalised
		const float invs = 1.0f / (sqx + sqy + sqz + sqw);
		result.m_data[0] = (sqx - sqy - sqz + sqw) * invs;
		// since sqw + sqx + sqy + sqz =1/invs*invs
		result.m_data[5] = (-sqx + sqy - sqz + sqw) * invs;
		result.m_data[10] = (-sqx - sqy + sqz + sqw) * invs;

		float tmp1 = static_cast<float>(x * y);
		float tmp2 = static_cast<float>(z * w);
		result.m_data[4] = 2.0f * (tmp1 + tmp2) * invs;
		result.m_data[1] = 2.0f * (tmp1 - tmp2) * invs;

		tmp1 = static_cast<float>(x * z);
		tmp2 = static_cast<float>(y * w);
		result.m_data[8] = 2.0f * (tmp1 - tmp2) * invs;
		result.m_data[2] = 2.0f * (tmp1 + tmp2) * invs;
		tmp1 = static_cast<float>(y * z);
		tmp2 = static_cast<float>(x * w);
		result.m_data[9] = 2.0f * (tmp1 + tmp2) * invs;
		result.m_data[6] = 2.0f * (tmp1 - tmp2) * invs;

		return { result };
	}

	inline std::ostream& operator<<(std::ostream& p_stream,
		const Quaternion& p_quaternion)
	{
		p_stream << "(w: " << p_quaternion.w << "; x: " << p_quaternion.x << ", y: " <<
			p_quaternion.y <<
			", z: " << p_quaternion.z << ')';
		return { p_stream };
	}
}
#pragma endregion
