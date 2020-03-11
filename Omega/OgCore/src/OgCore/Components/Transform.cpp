#include <OgCore/Components/Transform.h>

const OgEngine::Transform* OgEngine::Transform::GetParent() const
{
	return _parent;
}

OgEngine::Transform::Transform()
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(Matrix4F::identity), _localMatrix(Matrix4F::identity), _position(Vector3F::zero), _localPosition(Vector3F::zero), _scale(Vector3F::one), _localScale(Vector3F::one), _rotation(0.0, 0.0, 0.0, 1.0), _localRotation(0.0, 0.0, 0.0, 1.0)
{
}

OgEngine::Transform::Transform(const Matrix4F& p_matrix)
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(Matrix4F::identity), _localMatrix(Matrix4F::identity), _position(Vector3F::zero), _localPosition(Vector3F::zero), _scale(Vector3F::one), _localScale(Vector3F::one), _rotation(0.0, 0.0, 0.0, 1.0), _localRotation(0.0, 0.0, 0.0, 1.0)
{
	_localMatrix = p_matrix;
}

OgEngine::Transform::Transform(const Transform& p_other)
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(p_other.worldMatrix), _localMatrix(p_other.localMatrix), _position(p_other.position), _localPosition(p_other.localPosition), _scale(p_other.scale), _localScale(p_other.localScale), _rotation(p_other.rotation), _localRotation(p_other.localRotation)
{
}

OgEngine::Transform::Transform(Transform&& p_other) noexcept
	: worldMatrix(_worldMatrix), localMatrix(_localMatrix), position(_position), scale(_scale), rotation(_rotation),
	_worldMatrix(p_other.worldMatrix), _localMatrix(p_other.localMatrix), _position(p_other.position), _localPosition(p_other.localPosition), _scale(p_other.scale), _localScale(p_other.localScale), _rotation(p_other.rotation), _localRotation(p_other.localRotation)
{
}

void OgEngine::Transform::Translate(const Vector3F& p_movement)
{
	SetPosition(_localPosition + p_movement);
}

void OgEngine::Transform::Scale(const Vector3F& p_scale)
{
	SetScale(Vector3F(_localScale.x * p_scale.x, _localScale.y * p_scale.y, _localScale.z * p_scale.z));
}

void OgEngine::Transform::Rotate(const Quaternion& p_rotation)
{
	SetRotation(_localRotation * p_rotation);
}

void OgEngine::Transform::SetPosition(const Vector3F& p_position)
{
	GenerateMatrices(p_position, _localRotation, _localScale);
}

void OgEngine::Transform::SetScale(const Vector3F& p_scale)
{
	GenerateMatrices(_localPosition, _localRotation, p_scale);
}

void OgEngine::Transform::SetRotation(const Quaternion& p_rotation)
{
	GenerateMatrices(_localPosition, p_rotation, _localScale);
}

void OgEngine::Transform::SetParent(Transform* p_parent)
{
	_parent = p_parent;
}

void OgEngine::Transform::SetWorldMatrix(const GPM::Matrix4F& p_worldMatrix)
{
	_worldMatrix = p_worldMatrix;

	DecomposeWorldMatrix();
}

Vector3F OgEngine::Transform::WorldForward() const
{
	return _rotation * Vector3F::forward;
}

Vector3F OgEngine::Transform::WorldUp() const
{
	return _rotation * Vector3F::up;
}

Vector3F OgEngine::Transform::WorldRight() const
{
	return _rotation * Vector3F::right;
}

Vector3F OgEngine::Transform::LocalForward() const
{
	return _localRotation * Vector3F::forward;
}

Vector3F OgEngine::Transform::LocalUp() const
{
	return _localRotation * Vector3F::up;
}

Vector3F OgEngine::Transform::LocalRight() const
{
	return _localRotation * Vector3F::right;
}

OgEngine::Transform& OgEngine::Transform::operator=(const Transform& p_other)
{
	if (&p_other == this)
		return *this;

	_worldMatrix = p_other._worldMatrix;
	_localMatrix = p_other._localMatrix;
	
	_position = p_other._position;
	_localPosition = p_other.localPosition;
	_scale = p_other._scale;
	_localScale = p_other.localScale;
	_rotation = p_other._rotation;
	_localRotation = p_other.localRotation;
	
	return *this;
}

OgEngine::Transform& OgEngine::Transform::operator=(Transform&& p_other) noexcept
{
	_worldMatrix = p_other._worldMatrix;
	_localMatrix = p_other._localMatrix;
	
	_position = p_other._position;
	_localPosition = p_other.localPosition;
	_scale = p_other._scale;
	_localScale = p_other.localScale;
	_rotation = p_other._rotation;
	_localRotation = p_other.localRotation;

	return *this;
}

void OgEngine::Transform::GenerateMatrices(const Vector3F& p_position, const Quaternion& p_rotation,
		const Vector3F& p_scale)
{
	_localMatrix = Matrix4F::CreateTranslation(p_position) * Quaternion::Normalize(p_rotation).ToMatrix4() * Matrix4F::CreateScale(p_scale);
	_localPosition = p_position;
	_localRotation = p_rotation;
	_localScale = p_scale;
}

void OgEngine::Transform::DecomposeWorldMatrix()
{
	_position.x = _worldMatrix(0, 3);
	_position.y = _worldMatrix(1, 3);
	_position.z = _worldMatrix(2, 3);

	Vector3F columns[3] =
	{
		{ _worldMatrix(0, 0), _worldMatrix(1, 0), _worldMatrix(2, 0)},
		{ _worldMatrix(0, 1), _worldMatrix(1, 1), _worldMatrix(2, 1)},
		{ _worldMatrix(0, 2), _worldMatrix(1, 2), _worldMatrix(2, 2)},
	};

	_scale.x = columns[0].Magnitude();
	_scale.y = columns[1].Magnitude();
	_scale.z = columns[2].Magnitude();

	if (_scale.x)
	{
		columns[0] /= _scale.x;
	}
	if (_scale.y)
	{
		columns[1] /= _scale.y;
	}
	if (_scale.z)
	{
		columns[2] /= _scale.z;
	}

	const Matrix3F rotationMatrix
	(
		columns[0].x, columns[1].x, columns[2].x,
		columns[0].y, columns[1].y, columns[2].y,
		columns[0].z, columns[1].z, columns[2].z
	);

	_rotation = Quaternion(rotationMatrix);
}

std::ostream& OgEngine::operator<<(std::ostream& p_out, const Transform& p_other)
{
	p_out << "Transform:\nWorld matrix: \n" << p_other.worldMatrix << "Local matrix: \n" << p_other.localMatrix << "Position: " << p_other.position << "\nScale: " << p_other.scale << "\nRotation: " << p_other.rotation.ToEuler() << '\n';
	return p_out;
}
