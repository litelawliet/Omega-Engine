#pragma once
#include <OgRendering/Export.h>
#include <glm/glm.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>
#include <GPM/GPM.h>

#define GPM_USE
//#define GLM_USE

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
			GPM::Matrix4F rotMat = GPM::Matrix4F::identity;
			rotMat = GPM::Matrix4F::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.x), GPM::Vector3F(1.0f, 0, 0));
			rotMat = GPM::Matrix4F::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.y), GPM::Vector3F(0, 1.0f, 0));
			rotMat = GPM::Matrix4F::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.z), GPM::Vector3F(0, 0, 1.0f));

			GPM::Matrix4F transMat = GPM::Matrix4F::CreateTranslation(
				GPM::Vector3F(position.x, position.y, position.z));

			const Vector4F fd = rotMat * Vector4F(0, 0, 1, 0);
			forward = Vector3F(fd.x, fd.y, fd.z);

			const Vector4F u = rotMat * Vector4F(0, 1, 0, 0);
			up = Vector3F(u.x, u.y, u.z);

			right = Vector3F::Cross(forward, up);
			matrices.view = GPM::Matrix4F::LookAt(position, position + Vector3F(fd.x, fd.y, fd.z), Vector3F(u.x, u.y, u.z));
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
			matrices.perspective = GPM::Matrix4F::Perspective(p_fov, p_aspect, p_znear, p_zfar);
			//matrices.perspective(1, 1) *= -1;
			this->fov = p_fov;
			this->znear = p_znear;
			this->zfar = p_zfar;

			UpdateViewMatrix();
		}

		void SetPosition(const GPM::Vector3F& p_position)
		{
			this->position = p_position;
			UpdateViewMatrix();
		}

		void SetRotation(const GPM::Vector3F& p_rotation)
		{
			this->rotation = p_rotation;
			UpdateViewMatrix();
		};

		void Rotate(const GPM::Vector3F& p_delta)
		{
			this->rotation += p_delta;
			UpdateViewMatrix();
		}

		void Translate(const GPM::Vector3F& p_delta)
		{
			this->position += p_delta;
			UpdateViewMatrix();
		}

		struct
		{
			GPM::Matrix4F perspective;
			GPM::Matrix4F view;
		} matrices;

	public:
		float fov;
		float znear, zfar;

		GPM::Vector3F position{ 0, 0, 0 };
		GPM::Vector3F rotation{ 0, 0, 0 };
		Vector3F forward{ 0, 0, 1 };
		Vector3F up{ 0, 1, 0 };
		Vector3F right{ 1, 0, 0 };

		bool DOF;
		bool useGI;
		int bounceCount;
	};
}