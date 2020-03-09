#include <OgCore/Components/Transform.h>

const OgEngine::Transform* OgEngine::Transform::GetParent() const
{
	return _parent;
}

OgEngine::Transform::Transform()
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(Matrix4F::identity), _localMatrix(Matrix4F::identity), _position(Vector3F::zero), _scale(Vector3F::one), _rotation(0.0, 0.0, 0.0, 1.0)
{
}

OgEngine::Transform::Transform(const Matrix4F& p_matrix)
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(Matrix4F::identity), _localMatrix(Matrix4F::identity), _position(Vector3F::zero), _scale(Vector3F::one), _rotation(0.0, 0.0, 0.0, 1.0)
{
	_localMatrix = p_matrix;
}

OgEngine::Transform::Transform(const Transform& p_other)
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(p_other.worldMatrix), _localMatrix(p_other.localMatrix), _position(p_other.position), _scale(p_other.scale), _rotation(p_other.rotation)
{
}

OgEngine::Transform::Transform(Transform&& p_other) noexcept
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(p_other.worldMatrix), _localMatrix(p_other.localMatrix), _position(p_other.position), _scale(p_other.scale), _rotation(p_other.rotation)
{
}

void OgEngine::Transform::Translate(const Vector3F& p_movement)
{
	_localMatrix.Translate(p_movement);

	_position = p_movement;
}

void OgEngine::Transform::Scale(const Vector3F& p_scale)
{
	_localMatrix.Scale(p_scale);

	_scale += p_scale;
}

void OgEngine::Transform::Rotate(const float p_angle, const Vector3F& p_axis)
{
	_localMatrix.Rotate(Quaternion::CreateFromAxisAngle(p_axis, Tools::Utils::ToRadians(p_angle)));

	_rotation = Quaternion(worldMatrix);
}

void OgEngine::Transform::SetPosition(const Vector3F& p_position)
{
	_worldMatrix(0, 3) = p_position.x;
	_worldMatrix(1, 3) = p_position.y;
	_worldMatrix(2, 3) = p_position.z;

	_position = p_position;
}

void OgEngine::Transform::SetScale(const Vector3F& p_scale)
{
	_localMatrix = Matrix4F::identity;
	_localMatrix *= Matrix4F::CreateScale(p_scale) * Matrix4F::CreateRotation(rotation) * Matrix4F::CreateTranslation(_position);

	_scale = p_scale;
}

void OgEngine::Transform::SetRotation(const float p_yaw, const float p_pitch, const float p_roll)
{
	Quaternion rotation;
	rotation.MakeFromEuler(p_yaw, p_pitch, p_roll);

	_localMatrix = Matrix4F::identity;
	_localMatrix *= Matrix4F::CreateScale(_scale) * Matrix4F::CreateRotation(rotation) * Matrix4F::CreateTranslation(_position);
	 
	_rotation = rotation;
}

void OgEngine::Transform::SetParent(Transform* p_parent)
{
	_parent = p_parent;
}

void OgEngine::Transform::SetWorldMatrix(const GPM::Matrix4F& p_worldMatrix)
{
	_worldMatrix = p_worldMatrix;
}

OgEngine::Transform& OgEngine::Transform::operator=(const Transform& p_other)
{
	if (&p_other == this)
		return *this;

	_worldMatrix = p_other._worldMatrix;
	_localMatrix = p_other._localMatrix;
	_position = p_other._position;
	_scale = p_other._scale;
	_rotation = p_other._rotation;

	return *this;
}

OgEngine::Transform& OgEngine::Transform::operator=(Transform&& p_other) noexcept
{
	_worldMatrix = p_other._worldMatrix;
	_localMatrix = p_other._localMatrix;
	_position = p_other._position;
	_scale = p_other._scale;
	_rotation = p_other._rotation;

	return *this;
}

std::ostream& OgEngine::operator<<(std::ostream& p_out, const Transform& p_other)
{
	p_out << "Transform:\nWorld matrix: \n" << p_other.worldMatrix << "Local matrix: \n" << p_other.localMatrix << "Position: " << p_other.position << "\nScale: " << p_other.scale << "\nRotation: " << p_other.rotation.ToEuler() << '\n';
	return p_out;
}
