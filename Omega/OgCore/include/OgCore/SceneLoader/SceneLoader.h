#pragma once
#include <OgCore/Export.h>
#include <string>
#include <vector>
#include <OgCore/Export.h>
#include <glm/glm.hpp>
#include <sstream>

namespace OgEngine
{
	struct CORE_API SceneLoader
	{
		static std::string ExtractNameFromAttribute(const std::string& p_attributeLine);
		static float ExtractFloatFromAttribute(const std::string& p_attributeLine);
		static int ExtractIntegerFromAttribute(const std::string& p_attributeLine);
		static bool ExtractBooleanFromAttribute(const std::string& p_attributeLine);
		static glm::vec3 ExtractVector3FromAttribute(const std::string& p_attributeLine);
		static glm::vec4 ExtractVector4FromAttribute(const std::string& p_attributeLine);
		static bool SceneFileIntegrityCheck(const std::string& p_file);

	private:
		static int StringToInt(const std::string_view p_stringValue);
		static float StringToFloat(const std::string_view p_stringValue);
		static std::string ExtractDataFromAttribute(const std::string& p_attributeLine);

		static std::vector<std::string> symbolsList;
		
		static bool IsPair(const std::string& p_openingSymbol, const std::string& p_closingSymbol);
	};
}