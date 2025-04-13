#include "URLEncoder.h"
std::string RbsLib::Encoding::URLEncoder::Encode(const std::string& str)
{
	//遍历两遍，第一遍计算长度，第二遍填充，减少内存分配次数
	size_t len = 0;
	for (char c : str)
	{
		if (
			('A' <= c and c <= 'Z') or
			('a' <= c and c <= 'z') or
			('0' <= c and c <= '9') or
			(
				c == '-' or
				c == '_' or
				c == '.' or
				c == '!' or
				c == '~' or
				c == '*' or
				c == '\'' or
				c == ','))
		{
			len++;
		}
		else
		{
			len += 3;
		}
	}
	std::string result(len, 0);
	size_t i = 0;
	for (char c : str)
	{
		if (
			('A' <= c and c <= 'Z') or
			('a' <= c and c <= 'z') or
			('0' <= c and c <= '9') or
			(
				c == '-' or
				c == '_' or
				c == '.' or
				c == '!' or
				c == '~' or
				c == '*' or
				c == '\'' or
				c == ','))
		{
			result[i++] = c;
		}
		else
		{
			result[i++] = '%';
			result[i++] = "0123456789ABCDEF"[(unsigned char)c >> 4];
			result[i++] = "0123456789ABCDEF"[(unsigned char)c & 0x0F];
		}
	}
	return result;
}

std::string RbsLib::Encoding::URLEncoder::Decode(const std::string& str)
{
	std::string result;
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '%')
		{
			if (i + 2 < str.length())
			{
				char c = (char)((std::stoi(str.substr(i + 1, 2), nullptr, 16)) & 0xFF);
				result += c;
				i += 2;
			}
		}
		else if (str[i] == '+')
		{
			result += ' ';
		}
		else
		{
			result += str[i];
		}
	}
	return result;
}
