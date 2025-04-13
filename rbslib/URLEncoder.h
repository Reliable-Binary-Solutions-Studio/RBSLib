#pragma once
#include <string>
namespace RbsLib::Encoding
{
	class URLEncoder
	{
	public:
		static std::string Encode(const std::string& str);
		static std::string Decode(const std::string& str);
	};
}