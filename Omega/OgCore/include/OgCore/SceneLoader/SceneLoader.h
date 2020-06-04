#pragma once
#include <OgCore/Export.h>
#include <string>
#include <OgCore/Export.h>
#include <GPM/GPM.h>

namespace OgEngine
{
	struct CORE_API SceneLoader
	{
		static std::string ExtractNameFromAttribute(const std::string& p_attributeLine);
		static float ExtractFloatFromAttribute(const std::string& p_attributeLine);
		static int ExtractIntegerFromAttribute(const std::string& p_attributeLine);
		static bool ExtractBooleanFromAttribute(const std::string& p_attributeLine);
		static GPM::Vector3F ExtractVector3FromAttribute(const std::string& p_attributeLine);
		static GPM::Vector4F ExtractVector4FromAttribute(const std::string& p_attributeLine);
		static bool SceneFileIntegrityCheck(const std::string& p_file);

	private:
		static int StringToInt(const std::string_view p_stringValue);
		static float StringToFloat(const std::string_view p_stringValue);
		static std::string ExtractDataFromAttribute(const std::string& p_attributeLine);

		static std::vector<std::string> symbolsList;
		
		static bool IsPair(const std::string& p_openingSymbol, const std::string& p_closingSymbol);
	};
}