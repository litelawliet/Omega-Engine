#pragma once
#include <string_view>

template <typename T>
constexpr std::string_view type_name()
{
#ifdef __clang__
	const std::string_view p = __PRETTY_FUNCTION__;
	return std::string_view(p.data() + 34, p.size() - 34 - 1);
#elif defined(__GNUC__)
	const std::string_view p = __PRETTY_FUNCTION__;
#  if __cplusplus < 201402
	return std::string_view(p.data() + 36, p.size() - 36 - 1);
#  else
	return std::string_view(p.data() + 49, p.find(';', 49) - 49);
#  endif
#elif defined(_MSC_VER)
	const std::string_view p = __FUNCSIG__;
	return std::string_view(p.data() + 84, p.size() - 84 - 7);
#endif
}