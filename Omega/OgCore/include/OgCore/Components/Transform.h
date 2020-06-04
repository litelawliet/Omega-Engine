#pragma once
#include <OgCore/Export.h>
#include <GPM/GPM.h>

namespace OgEngine
{
	struct CORE_API Transform final
	{
		/**
		 *	@brief World transform of this gameObject (read-only)
		 */
		const Matrix4F& worldMatrix = _worldMatrix;

		/**
		*	@brief Local transform of this gameObject (read-only)
		*/
		const Matrix4F& localMatrix = _localMatrix;

		/**
		 *	@brief World position of this gameObject (read-only)
		 */
		const Vector3F& position = _position;

		/**
		 *	@brief Local position of this gameObject (read-only)
		 */
		const Vector3F& localPosition = _localPosition;

		/**
		 *	@brief World scale of this gameObject (read-only)
		 */
		const Vector3F& scale = _scale;

		/**
		 *	@brief Local scale of this gameObject (read-only)
		 */
		const Vector3F& localScale = _localScale;

		/**
		 *	@brief World rotation of this gameObject in radians (read-only)
		 */
		const Quaternion& rotation = _rotation;

		/**
		 *	@brief Local rotation of this gameObject in radians (read-only)
		 */
		const Quaternion& localRotation = _localRotation;

		/**
		 * @brief Name of the gameObject (read-only)
		*/
		const std::string& name = _name;

		/**
		*	@brief Default constructor
		*/
		Transform();
		/**
		*	@brief Constructor using a matrix
		*	@param p_matrix The matrix used to set the transform
		*/
		explicit Transform(Matrix4F p_matrix);

		/**
		*	@brief Copy constructor
		*	@param p_other The other transform
		*/
		Transform(const Transform& p_other);

		/**
		*	@brief Move constructor
		*	@param p_other The other transform
		*/
		Transform(Transform&& p_other) noexcept;

		/**
		*	@brief Destructor
		*/
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
		 * @param p_rotation The angle to add
		 */
		void Rotate(const Quaternion& p_rotation);

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
		 * @param p_rotation The rotation quaternion
		 */
		void SetRotation(const Quaternion& p_rotation);

		/**
		 * @brief Set the transform to a new rotation.
		 * @param p_parent The new parent of this transform
		 */
		void SetParent(Transform* p_parent);

		/**
		 * @brief Set the transform world matrix.
		 * @param p_worldMatrix The new parent of this transform
		 * @note This method should be used when updating the children of the scenegraph.
		 */
		void SetWorldMatrix(const GPM::Matrix4F& p_worldMatrix);

		/**
		 * @brief Set the GameObject's name
		 * @param p_name is the new name of the object
		*/
		void SetName(const std::string& p_name);

		[[nodiscard]] std::string Serialize(const int p_depth) const;

		/**
		 * @brief Return the world forward of this transform.
		 * @return The world forward
		 */
		[[nodiscard]] Vector3F WorldForward() const;

		/**
		 * @brief Return the world up of this transform.
		 * @return The world up
		 */
		[[nodiscard]] Vector3F WorldUp() const;

		/**
		 * @brief Return the world right of this transform.
		 * @return The world right
		 */
		[[nodiscard]] Vector3F WorldRight() const;

		/**
		 * @brief Return the local forward of this transform.
		 * @return The local forward
		 */
		[[nodiscard]] Vector3F LocalForward() const;

		/**
		 * @brief Return the local up of this transform.
		 * @return The local up
		 */
		[[nodiscard]] Vector3F LocalUp() const;

		/**
		 * @brief Return the local right of this transform.
		 * @return The local right
		 */
		[[nodiscard]] Vector3F LocalRight() const;

		/**
		* @brief Return parent transform (read-only). If no parent exists, returns nullptr
		* @return Transform* : The parent transform if existing
		*/
		[[nodiscard]] const Transform* GetParent() const;

		/**
		* @brief Tell if this Transform has a parent
		* @return True of False
		*/
		[[nodiscard]] bool HasParent() const;

		/**
		 * @brief Copy assignment
		 * @param p_other The other transform
		 * @return The other transform to copy
		 */
		Transform& operator=(const Transform& p_other);

		/**
		 * @brief Move assignment
		 * @param p_other The other transform
		 * @return The other transform to move
		 */
		Transform& operator=(Transform&& p_other) noexcept;


	private:
		alignas(64) Matrix4F _worldMatrix;
		alignas(64) Matrix4F _localMatrix;
		alignas(16) Vector3F _position;
		alignas(16) Vector3F _localPosition;
		alignas(16) Vector3F _scale;
		alignas(16) Vector3F _localScale;
		alignas(16) Quaternion _rotation;
		alignas(16) Quaternion _localRotation;
		alignas(8) Transform* _parent = nullptr;
		std::string _name;

		/**
		 * @brief Generate the local matrix using the locals position, rotation and scale.
		 * @param p_position The local position
		 * @param p_rotation The local rotation
		 * @param p_scale The local scale
		 */
		void GenerateMatrices(const Vector3F& p_position, const Quaternion& p_rotation, const Vector3F& p_scale);

		/**
		 * @brief Decompose the world matrix and set all the world position, rotation and scale out of it.
		 */
		void DecomposeWorldMatrix();

		[[nodiscard]] static std::string DepthIndent(const int p_depth);

	};

	std::ostream& operator<<(std::ostream& p_out, const Transform& p_other);
}
