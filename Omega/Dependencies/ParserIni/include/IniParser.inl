#pragma once

template <typename T>
constexpr T IniParser::Get(const std::string& p_key)
{
	if constexpr (std::is_same<bool, T>::value)
	{
		if (!KeyExist(p_key))
			return false;

		return StringToBoolean(m_data[p_key]);
	}
	else if constexpr (std::is_same<std::string, T>::value)
	{
		if (!KeyExist(p_key))
			return std::string("NULL");

		return m_data[p_key];
	}
	else if constexpr (std::is_integral<T>::value)
	{
		if (!KeyExist(p_key))
			return static_cast<T>(0);

		char* ptr;
		return static_cast<T>(std::strtol(m_data[p_key].c_str(), &ptr, 10));
	}
	else if constexpr (std::is_floating_point<T>::value)
	{
		if (!KeyExist(p_key))
			return static_cast<T>(0.0f);

		char* ptr;
		return static_cast<T>(std::strtod(m_data[p_key].c_str(), &ptr));
	}
	else
	{
		static_assert("The given type must be : bool, integral, floating point or string");
		return T();
	}
}

template<typename T>
constexpr bool IniParser::Add(const std::string& p_key, const T p_value)
{
	if (!KeyExist(p_key))
	{
		if constexpr (std::is_same<bool, T>::value)
		{
			RegisterPair(p_key, p_value ? "true" : "false");
		}
		else if constexpr (std::is_same<std::string, T>::value)
		{
			RegisterPair(p_key, p_value);
		}
		else if constexpr (std::is_integral<T>::value)
		{
			RegisterPair(p_key, std::to_string(p_value));
		}
		else if constexpr (std::is_floating_point<T>::value)
		{
			RegisterPair(p_key, std::to_string(p_value));
		}
		else
		{
			static_assert("The given type must be : bool, integral, floating point or std::string");
		}

		return true;
	}

	return false;
}

template<typename T>
constexpr bool IniParser::Set(const std::string& p_key, const T p_value)
{
	if (KeyExist(p_key))
	{
		if constexpr (std::is_same<bool, T>::value)
		{
			m_data[p_key] = p_value ? "true" : "false";
		}
		else if constexpr (std::is_same<std::string, T>::value)
		{
			m_data[p_key] = p_value;
		}
		else if constexpr (std::is_integral<T>::value)
		{
			m_data[p_key] = std::to_string(p_value);
		}
		else if constexpr (std::is_floating_point<T>::value)
		{
			m_data[p_key] = std::to_string(p_value);
		}
		else
		{
			static_assert("The given type must be : bool, integral, floating point or string");
		}

		return true;
	}

	return false;
}