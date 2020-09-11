#pragma once
#include <OgRendering/Export.h>
#include <glm/glm.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>
#include <GPM/GPM.h>

//#define GPM_USE
#define GLM_USE

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
#ifdef GPM_USE
			glm::mat4 rotMat = glm::mat4::identity;
			rotMat = glm::mat4::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.x), glm::vec3(1.0f, 0, 0));
			rotMat = glm::mat4::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.y), glm::vec3(0, 1.0f, 0));
			rotMat = glm::mat4::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.z), glm::vec3(0, 0, 1.0f));

			glm::mat4 transMat = glm::mat4::CreateTranslation(
				glm::vec3(position.x, position.y, position.z));

			const Vector4F fd = rotMat * Vector4F(0, 0, 1, 0);
			forward = glm::vec3(fd.x, fd.y, fd.z);

			const Vector4F u = rotMat * Vector4F(0, 1, 0, 0);
			up = glm::vec3(u.x, u.y, u.z);

			right = glm::vec3::Cross(forward, up);
			matrices.view = glm::mat4::LookAt(position, position + glm::vec3(fd.x, fd.y, fd.z), glm::vec3(u.x, u.y, u.z));
			//matrices.view(2,2) = -1;

#endif
#ifdef GLM_USE
			glm::mat4 rotM = glm::mat4(1.0f);

			rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			glm::mat4 transM = glm::translate(glm::mat4(1.0f), position);

			for (int i = 0; i < 4; ++i)
				std::cout << transM[i].x << '/' << transM[i].y << '/' << transM[i].z << '/' << transM[i].w << '\n';

			for (int i = 0; i < 4; ++i)
				std::cout << rotM[i].x << '/' << rotM[i].y << '/' << rotM[i].z << '/' << rotM[i].w << '\n';

			matrices.view = transM * rotM;
			matrices.view[2][2] = -1;
#endif
		}

		void SetPerspective(const float p_fov, const float p_aspect, const float p_znear, const float p_zfar)
		{
			matrices.perspective = glm::perspective(p_fov, p_aspect, p_znear, p_zfar);
			//matrices.perspective(1, 1) *= -1;
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