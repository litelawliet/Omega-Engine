#pragma once

#include <GPM/GPM.h>

#include <OgCore/Components/Transform.h>

namespace OgEngine
{

	struct Material
	{
    public:
        void SetColor(const GPM::Vector4F p_color) { color = p_color; }

        /**
         * @brief Roughness value of the material in the range [0,1]
         */
        void SetRoughness(const float p_roughness) { roughness = p_roughness; }
        void SetMetallic(const float p_metallic) { metallic = p_metallic; }
        void SetReflectance(const float p_reflectance) { reflectance = p_reflectance; }

        /**
         * @brief this is the Material type defined by an int
         * 0 -> Conductor
         * 1 -> Dielectric
         * 2 -> Emissive
        */
        void SetType(const int p_type) { materialType = p_type; }


		Material();
		
        const GPM::Vector4F Color() const { return color; }
        const float Reflectance() const { return reflectance; }
        const float Roughness() const { return roughness; }
        const float Metallic() const { return metallic; }
        const int Type() const { return materialType; }

		void SetLocalTransform(Transform& p_transform);

	private:
		Transform* m_lightTransform{};

		GPM::Vector4F color;
		float metallic{ 0 };
		float roughness{ 0 };
        float reflectance{ 0 };
        int materialType{ 1 };
	};
}
