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

        Camera() : fov(60.0f), znear(0.1f), zfar(512.0f) {}

        void updateViewMatrix()
        {
#ifdef GPM_USE
            GPM::Matrix4F rotMat = GPM::Matrix4F::identity;
            rotMat = GPM::Matrix4F::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.x), GPM::Vector3F(1.0f, 0, 0));
            rotMat = GPM::Matrix4F::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.y), GPM::Vector3F(0, 1.0f, 0));
            rotMat = GPM::Matrix4F::Rotate(rotMat, GPM::Tools::Utils::ToRadians(rotation.z), GPM::Vector3F(0, 0, 1.0f));

            GPM::Matrix4F transMat = GPM::Matrix4F::identity;
            transMat = GPM::Matrix4F::CreateTranslation(GPM::Vector3F(position.x, position.y, position.z));
                
            //matrices.view = transMat * rotMat;
            matrices.view = GPM::Matrix4F::LookAt(position, Vector3F(0, 3, 0), Vector3F(0, 1, 0));
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
        };

        void setPerspective(float fov, float aspect, float znear, float zfar)
        {
            matrices.perspective = GPM::Matrix4F::Perspective(fov, aspect, znear, zfar);
            //matrices.perspective(1, 1) *= -1;
            this->fov = fov;
            this->znear = znear;
            this->zfar = zfar;

            updateViewMatrix();
        };

        void setPosition(GPM::Vector3F position)
        {
            this->position = position;
            updateViewMatrix();
        }

        void setRotation(GPM::Vector3F rotation)
        {
            this->rotation = rotation;
            updateViewMatrix();
        };

        void rotate(GPM::Vector3F delta)
        {
            this->rotation += delta;
            updateViewMatrix();
        }

        void translate(GPM::Vector3F delta)
        {
            this->position += delta;
            updateViewMatrix();
        }

    struct
    {
        GPM::Matrix4F perspective;
        GPM::Matrix4F view;
    } matrices;

    private:
        float fov;
        float znear, zfar;

        GPM::Vector3F position{ 0, 0, 0 };
        GPM::Vector3F rotation{ 0, 0, 0 };
    };
}