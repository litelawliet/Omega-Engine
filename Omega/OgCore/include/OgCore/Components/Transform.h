#pragma once

#include <GPM/GPM.h>

namespace OgEngine
{	
	struct Transform final
	{
		/**
		 *	@brief transform of this gameObject (read-only)
		 */
		const Matrix4F& worldMatrix = _worldMatrix;

		/**
		*	@brief local transform of this gameObject (read-only)
		*/
		const Matrix4F& localMatrix = _localMatrix;

		/**
		 *	@brief position of this gameObject (read-only)
		 */
		const Vector3F& position = _position;

		/**
		 *	@brief scale of this gameObject (read-only)
		 */
		const Vector3F& scale = _scale;

		/**
		 *	@brief rotation of this gameObject in radians (read-only)
		 */
		const Quaternion& rotation = _rotation;

		/**
		*	@brief Return parent transform (read-only). If no parent exists, returns nullptr
		*/
		[[nodiscard]] const Transform* GetParent() const;
		
		Transform();
		explicit Transform(const Matrix4F& p_matrix);
		Transform(const Transform& p_other);
		Transform(Transform&& p_other) noexcept;
		~Transform() = default;

		/**
		 * @brief Move the current transform of a certain movement.
		 * @param p_movement Movement to add to the current transform
		 */
		void Translate(const Vector3F& p_movement);

		/**
		 * @brief Scale or Unscale the current transform of a certain amount.
		 * @param p_scale The new scale
		 */
		void Scale(const Vector3F& p_scale);

		/**
		 * @brief Rotate the transform of a certain angle (in degree).
		 * @param p_angle The angle to add
		 * @param p_axis The axe of rotation
		 */
		void Rotate(const float p_angle, const Vector3F& p_axis);

		/**
		 * @brief Set the transform to a new position in world space.
		 * @param p_position The new position in world space
		 */
		void SetPosition(const Vector3F& p_position);

		/**
		 * @brief Set the transform to a new scale.
		 * @param p_scale The new scale
		 */
		void SetScale(const Vector3F& p_scale);

		/**
		 * @brief Set the transform to a new rotation.
		 * @param p_yaw The new right axis angle
		 * @param p_pitch The new up axis angle
		 * @param p_roll The new forward axis angle
		 */
		void SetRotation(const float p_yaw, const float p_pitch, const float p_roll);

		/**
		 * @brief Set the transform to a new rotation.
		 * @param p_parent The new parent of this transform
		 */
		void SetParent(Transform* p_parent);

		/**
		 * @brief Set the transform world matrix.
		 * @param p_worldMatrix The new parent of this transform
		 */
		void SetWorldMatrix(const GPM::Matrix4F& p_worldMatrix);
		
		Transform& operator=(const Transform& p_other);
		Transform& operator=(Transform&& p_other) noexcept;


	private:
		alignas(64) Matrix4F _worldMatrix;
		alignas(64) Matrix4F _localMatrix;
		alignas(16) Vector3F _position;
		alignas(16) Vector3F _scale;
		alignas(16) Quaternion _rotation;
		alignas(16) Transform* _parent = nullptr;
	};

	std::ostream& operator<<(std::ostream& p_out, const Transform& p_other);
}
