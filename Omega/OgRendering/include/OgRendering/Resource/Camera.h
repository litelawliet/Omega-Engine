#pragma once
#include <OgRendering/Export.h>
#include <cstdint>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace OgEngine
{
	class RENDERING_API Camera
	{
	public:

		Camera() : fov(60.0f), znear(0.1f), zfar(1000.0f)
		{
		}

		void UpdateViewMatrix()
		{
			glm::mat4 rotMat = glm::mat4(1.0f);
			rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1.0f, 0, 0));
			rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0, 1.0f, 0));
			rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0, 0, 1.0f));

			const glm::vec4 fd = rotMat * glm::vec4(0, 0, 1, 0);
			forward = glm::vec3(fd.x, fd.y, fd.z);

			const glm::vec4 u = rotMat * glm::vec4(0, 1, 0, 0);
			up = glm::vec3(u.x, u.y, u.z);

			right = -glm::cross(glm::normalize(forward), glm::normalize(up));
			matrices.view = glm::lookAt(position, position + glm::vec3(fd.x, fd.y, fd.z), glm::vec3(u.x, u.y, u.z));
		}

		void SetPerspective(const float p_fov, const float p_aspect, const float p_znear, const float p_zfar)
		{
			matrices.perspective = glm::perspective(glm::radians(p_fov), p_aspect, p_znear, p_zfar);
			matrices.perspective[1][1] *= -1.0f;
			this->fov = p_fov;
			this->znear = p_znear;
			this->zfar = p_zfar;

			UpdateViewMatrix();
		}

		void SetPosition(const glm::vec3& p_position)
		{
			this->position = p_position;
			UpdateViewMatrix();
		}

		void SetRotation(const glm::vec3& p_rotation)
		{
			this->rotation = p_rotation;
			UpdateViewMatrix();
		};

		void Rotate(const glm::vec3& p_delta)
		{
			this->rotation += p_delta;
			UpdateViewMatrix();
		}

		void Translate(const glm::vec3& p_delta)
		{
			this->position += p_delta;
			UpdateViewMatrix();
		}

		struct
		{
			glm::mat4 perspective;
			glm::mat4 view;
		} matrices;

	public:
		float fov;
		float znear, zfar;

		glm::vec3 position{ 0, 0, 0 };
		glm::vec3 rotation{ 0, 0, 0 };
		glm::vec3 forward{ 0, 0, 1 };
		glm::vec3 up{ 0, 1, 0 };
		glm::vec3 right{ 1, 0, 0 };

		bool DOF;
		bool useGI;
		int bounceCount;
	};
}