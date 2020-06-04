#include <stack>
#include <fstream>
#include <OgCore/SceneLoader/SceneLoader.h>

std::vector<std::string> OgEngine::SceneLoader::symbolsList = {
	"<SceneNode>", "<Transform>", "<Model>", "<Material>", "<RigidBody>", "<LightSource>",
		"</SceneNode>", "</Transform>", "</Model>", "</Material>", "</RigidBody>", "</LightSource>"
};

std::string OgEngine::SceneLoader::ExtractNameFromAttribute(const std::string& p_attributeLine)
{
	return ExtractDataFromAttribute(p_attributeLine);
}

float OgEngine::SceneLoader::ExtractFloatFromAttribute(const std::string& p_attributeLine)
{
	return StringToFloat(ExtractDataFromAttribute(p_attributeLine));
}

int OgEngine::SceneLoader::ExtractIntegerFromAttribute(const std::string& p_attributeLine)
{
	return StringToInt(ExtractDataFromAttribute(p_attributeLine));
}

bool OgEngine::SceneLoader::ExtractBooleanFromAttribute(const std::string& p_attributeLine)
{
	return StringToInt(ExtractDataFromAttribute(p_attributeLine));
}

GPM::Vector3F OgEngine::SceneLoader::ExtractVector3FromAttribute(const std::string& p_attributeLine)
{
	const std::string data = ExtractDataFromAttribute(p_attributeLine);

	Vector3F returnedVector;
	std::istringstream valueStream(data);
	std::string readValue;

	// Read x
	std::getline(valueStream, readValue, ';');
	returnedVector.x = StringToFloat(readValue);

	// Read y
	std::getline(valueStream, readValue, ';');
	returnedVector.y = StringToFloat(readValue);

	// Read z
	std::getline(valueStream, readValue);
	returnedVector.z = StringToFloat(readValue);

	return returnedVector;
}

GPM::Vector4F OgEngine::SceneLoader::ExtractVector4FromAttribute(const std::string& p_attributeLine)
{
	const std::string data = ExtractDataFromAttribute(p_attributeLine);

	Vector4F returnedVector;
	std::istringstream valueStream(data);
	std::string readValue;

	// Read x
	std::getline(valueStream, readValue, ';');
	returnedVector.x = StringToFloat(readValue);

	// Read y
	std::getline(valueStream, readValue, ';');
	returnedVector.y = StringToFloat(readValue);

	// Read z
	std::getline(valueStream, readValue, ';');
	returnedVector.z = StringToFloat(readValue);

	// Read w
	std::getline(valueStream, readValue);
	returnedVector.w = StringToFloat(readValue);

	return returnedVector;
}

float OgEngine::SceneLoader::StringToFloat(const std::string_view p_stringValue)
{
	float value;
	try
	{
		value = std::stof(p_stringValue.data());
	}
	catch (const std::invalid_argument& p_exception)
	{
		throw std::runtime_error(p_exception.what());
	}
	catch (const std::out_of_range& p_exception)
	{
		throw std::runtime_error(p_exception.what());
	}

	return value;
}

int OgEngine::SceneLoader::StringToInt(const std::string_view p_stringValue)
{
	int value;
	try
	{
		value = std::stoi(p_stringValue.data());
	}
	catch (const std::invalid_argument& p_exception)
	{
		throw std::runtime_error(p_exception.what());
	}
	catch (const std::out_of_range& p_exception)
	{
		throw std::runtime_error(p_exception.what());
	}

	return value;
}

std::string OgEngine::SceneLoader::ExtractDataFromAttribute(const std::string& p_attributeLine)
{
	const size_t index = p_attributeLine.find_first_of('>') + 1u;
	std::string data = p_attributeLine.substr(index);
	std::istringstream valueStream(data);

	std::getline(valueStream, data, '<'); // Went all the way to the key marker

	return data;
}

bool OgEngine::SceneLoader::IsPair(const std::string& p_openingSymbol, const std::string& p_closingSymbol)
{
	return (p_openingSymbol == "<SceneNode>" && p_closingSymbol == "</SceneNode>")
		|| (p_openingSymbol == "<Transform>" && p_closingSymbol == "</Transform>")
		|| (p_openingSymbol == "<Model>" && p_closingSymbol == "</Model>")
		|| (p_openingSymbol == "<Material>" && p_closingSymbol == "</Material>")
		|| (p_openingSymbol == "<RigidBody>" && p_closingSymbol == "</RigidBody>")
		|| (p_openingSymbol == "<LightSource>" && p_closingSymbol == "</LightSource>");
}

bool OgEngine::SceneLoader::SceneFileIntegrityCheck(const std::string& p_file)
{
	std::ifstream file;
	file.open(p_file, std::ios::in);
	std::stack<std::string>  s;

	if (file.is_open())
	{
		std::string line;
		while (!file.eof())
		{
			std::getline(file, line);
			
			// Remove all useless indentation
			const size_t indexBegin = line.find_first_of('<');
			const size_t indexEnd = line.find_first_of('>') + 1u;
			std::string data;
			if (indexBegin != std::string::npos && indexEnd != std::string::npos)
			{
				data = line.substr(indexBegin, indexEnd - indexBegin);
			}
			else
			{
				data = line;
			}

			// If found in symbol lists, then we check if it's opening and closing correctly
			if (std::find(symbolsList.begin(), symbolsList.end(), data) != symbolsList.end())
			{
				if (data == "<SceneNode>" || data == "<Transform>" || data == "<Model>" || data == "<Material>" || data == "<RigidBody>" || data == "<LightSource>")
					s.push(data);
				else if (data == "</SceneNode>" || data == "</Transform>" || data == "</Model>" || data == "</Material>" || data == "</RigidBody>" || data == "</LightSource>")
				{
					if (s.empty() || !IsPair(s.top(), data))
						return false;
					else
						s.pop();
				}
			}
		}
	}
	else
	{
		// File couldn't be read, therefore wrong file
		return false;
	}

	return s.empty();
}