#pragma once
#include <string>
#include <unordered_map>

class IniParser final
{
public:
	IniParser(const char* p_file, const bool p_loadFile = true);
	~IniParser();

	bool Load();
	bool Load(const char* p_file);
	void Reload();

	bool KeyExist(const std::string& p_key);

	bool Save();

	bool Remove(const std::string& p_key);
	
	template<typename T>
	constexpr T Get(const std::string& p_key);

	template<typename T>
	constexpr bool Add(const std::string& p_key, const T p_value);

	template<typename T>
	constexpr bool Set(const std::string& p_key, const T p_value);
	
private:
	std::string m_filePath;
	std::unordered_map<std::string, std::string> m_data;

	void RegisterPair(const std::string& p_key, const std::string& p_value);
	void RegisterPair(const std::pair<std::string, std::string>& p_pair);

	static bool IsValidLine(const std::string& p_attributeLine);
	static std::pair<std::string, std::string> ExtractKeyValue(const std::string& p_line);
	static bool StringToBoolean(const std::string& p_value);
};

#include <IniParser.inl>
