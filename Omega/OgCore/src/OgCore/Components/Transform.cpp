#include <OgCore/Components/Transform.h>
#include <fstream>
#include <utility>

OgEngine::Transform::Transform()
	: _worldMatrix(glm::mat4(1.0f)), _localMatrix(glm::mat4(1.0f)),
	_position(glm::vec3(0)), _localPosition(glm::vec3(0)),
	_scale(glm::vec3(1)), _localScale(glm::vec3(1)), _editorRotation(glm::vec3(0)),
	_rotation(0.0, 0.0, 0.0, 1.0), _localRotation(0.0, 0.0, 0.0, 1.0)
{
	_name.resize(32);
}

OgEngine::Transform::Transform(glm::mat4 p_matrix)
	:
	_worldMatrix(glm::mat4(1.0f)), _localMatrix(p_matrix),
	_position(glm::vec3(0)), _localPosition(glm::vec3(0)),
	_scale(glm::vec3(1)), _localScale(glm::vec3(1)), _editorRotation(glm::vec3(0)),
	_rotation(0.0, 0.0, 0.0, 1.0), _localRotation(0.0, 0.0, 0.0, 1.0)
{
	_name.resize(32);
}

OgEngine::Transform::Transform(const Transform& p_other)
	:
	_worldMatrix(p_other.worldMatrix), _localMatrix(p_other.localMatrix),
	_position(p_other.position), _localPosition(p_other.localPosition),
	_scale(p_other.scale), _localScale(p_other.localScale), _editorRotation(p_other.editorRotation),
	_rotation(p_other.rotation), _localRotation(p_other.localRotation),
	_name(p_other._name)
{
}

OgEngine::Transform::Transform(Transform&& p_other) noexcept
	: _worldMatrix(std::move(p_other._worldMatrix)), _localMatrix(std::move(p_other._localMatrix)),
	_position(p_other.position), _localPosition(p_other.localPosition),
	_scale(p_other.scale), _localScale(p_other.localScale), _editorRotation(p_other.editorRotation),
	_rotation(p_other.rotation), _localRotation(p_other.localRotation),
	_name(p_other._name)
{
}

void OgEngine::Transform::Translate(const glm::vec3& p_movement)
{
	SetPosition(_localPosition + p_movement);
}

void OgEngine::Transform::Scale(const glm::vec3& p_scale)
{
	SetScale(glm::vec3(_localScale.x * p_scale.x, _localScale.y * p_scale.y, _localScale.z * p_scale.z));
}

void OgEngine::Transform::Rotate(const glm::quat& p_rotation)
{
	SetRotation(_localRotation * p_rotation);
}

void OgEngine::Transform::SetPosition(const glm::vec3& p_position)
{
	GenerateMatrices(p_position, _localRotation, _localScale);
}

void OgEngine::Transform::SetScale(const glm::vec3& p_scale)
{
	GenerateMatrices(_localPosition, _localRotation, p_scale);
}

void OgEngine::Transform::SetEditorRotation(const glm::vec3& p_editorRotation)
{
	_editorRotation = p_editorRotation;
}

void OgEngine::Transform::SetRotation(const glm::quat& p_rotation)
{
	GenerateMatrices(_localPosition, p_rotation, _localScale);
}

void OgEngine::Transform::SetParent(Transform* p_parent)
{
	_parent = p_parent;
}

void OgEngine::Transform::SetWorldMatrix(const glm::mat4& p_worldMatrix)
{
	_worldMatrix = p_worldMatrix;

	DecomposeWorldMatrix();
}

void OgEngine::Transform::SetName(const std::string& p_name)
{
	_name = p_name;
}

std::string OgEngine::Transform::Serialize(const int p_depth) const
{
	return std::string(DepthIndent(p_depth) + "<Transform>\n"
		+ DepthIndent(p_depth + 1) + "<name>" + _name + "</name>\n"
		+ DepthIndent(p_depth + 1) + "<position>" + std::to_string(_localPosition.x) + ";" + std::to_string(_localPosition.y) + ";" + std::to_string(_localPosition.z) + "</position>\n"
		+ DepthIndent(p_depth + 1) + "<rotation>" + std::to_string(_localRotation.x) + ";" + std::to_string(_localRotation.y) + ";" + std::to_string(_localRotation.z) + ";" + std::to_string(_localRotation.w) + "</rotation>\n"
		+ DepthIndent(p_depth + 1) + "<scale>" + std::to_string(_localScale.x) + ";" + std::to_string(_localScale.y) + ";" + std::to_string(_localScale.z) + "</scale>\n"
		+ DepthIndent(p_depth) + "</Transform>\n");
}

glm::vec3 OgEngine::Transform::WorldForward() const
{
	return _rotation * glm::vec4(0,0,1,1);
}

glm::vec3 OgEngine::Transform::WorldUp() const
{
	return _rotation * glm::vec4(0,1,0, 1);
}

glm::vec3 OgEngine::Transform::WorldRight() const
{
	return _rotation * glm::vec4(1,0,0,1);
}

glm::vec3 OgEngine::Transform::LocalForward() const
{
	return _localRotation * glm::vec4(0,0,1,1);
}

glm::vec3 OgEngine::Transform::LocalUp() const
{
	return _localRotation * glm::vec4(0,1,0, 1);
}

glm::vec3 OgEngine::Transform::LocalRight() const
{
	return _localRotation * glm::vec4(1,0,0,1);
}

const OgEngine::Transform* OgEngine::Transform::GetParent() const
{
	return _parent;
}

bool OgEngine::Transform::HasParent() const
{
	return _parent != nullptr;
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
	_name = p_other.name;

	return *this;
}

OgEngine::Transform& OgEngine::Transform::operator=(Transform&& p_other) noexcept
{
	_worldMatrix = std::move(p_other._worldMatrix);
	_localMatrix = std::move(p_other._localMatrix);

	_position = p_other._position;
	_localPosition = p_other.localPosition;
	_scale = p_other._scale;
	_localScale = p_other.localScale;
	_rotation = p_other._rotation;
	_localRotation = p_other.localRotation;
	_name = std::move(p_other._name);

	return *this;
}

void OgEngine::Transform::GenerateMatrices(const glm::vec3& p_position, const glm::quat& p_rotation,
	const glm::vec3& p_scale)
{
	const glm::mat4 tr = glm::translate(glm::mat4(1.0), p_position);
	const glm::mat4 ro = glm::toMat4(p_rotation);

	const glm::mat4 sc = glm::scale(glm::mat4(1.0), p_scale);
	_localMatrix = tr * ro * sc;
	
	_localPosition = p_position;
	_localRotation = p_rotation;
	_localScale = p_scale;
}

void OgEngine::Transform::DecomposeWorldMatrix()
{
	glm::vec3 skew = glm::vec3(0.0f);
	glm::vec4 perspective = glm::vec4(0.0f);
	glm::decompose(_worldMatrix, _scale, _rotation, _position, skew, perspective);
}

std::string OgEngine::Transform::DepthIndent(const int p_depth)
{
	std::string depthCode;
	for (auto i = 0; i < p_depth; ++i)
	{
		depthCode += "\t";
	}

	return depthCode;
}

std::ostream& OgEngine::operator<<(std::ostream& p_out, const Transform& p_other)
{
	p_out << "Transform of " << p_other.name << ":\nWorld matrix: \n" << glm::to_string(p_other.worldMatrix) << "Local matrix: \n" << glm::to_string(p_other.localMatrix) << "Position: " << glm::to_string(p_other.position) << "\nScale: " << glm::to_string(p_other.scale) << "\nRotation: " << glm::to_string(glm::degrees(glm::eulerAngles(p_other.rotation))) << '\n';
	return p_out;
}
