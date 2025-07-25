#include "CharsetConvert.h"
#include "BaseType.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef WIN32
std::string RbsLib::Encoding::CharsetConvert::UTF8toANSI(const std::string& strUTF8)
{
	UINT nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, NULL, NULL);
	WCHAR* wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR* szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;
	std::string str = szBuffer;
	delete[]szBuffer;
	delete[]wszBuffer;
	return str;
}

std::string RbsLib::Encoding::CharsetConvert::ANSItoUTF8(const std::string& strAnsi)
{
	UINT nLen = MultiByteToWideChar(936, NULL, strAnsi.c_str(), -1, NULL, NULL);
	WCHAR* wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(936, NULL, strAnsi.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR* szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;
	std::string str = szBuffer;
	delete[]wszBuffer;
	delete[]szBuffer;
	return str;
}

#endif

#ifdef LINUX
#include <iconv.h>
#include <cstring>
#include <stdexcept>

static std::string convert_encoding(const std::string& input, const char* from_charset, const char* to_charset)
{
    iconv_t cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t)-1) {
        throw std::runtime_error("iconv_open failed");
    }

    size_t inbytesleft = input.size();
    size_t outbytesleft = inbytesleft * 4 + 4; // 预留足够空间
    std::string output(outbytesleft, 0);

    char* inbuf = const_cast<char*>(input.data());
    char* outbuf = output.data();
    char* outbuf_start = outbuf;

    if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1) {
        iconv_close(cd);
        throw std::runtime_error("iconv conversion failed");
    }
    iconv_close(cd);
    return std::string(outbuf_start, outbuf - outbuf_start);
}

std::string RbsLib::Encoding::CharsetConvert::UTF8toANSI(const std::string& strUTF8)
{
	return strUTF8;
    // "GBK" 代表简体中文 ANSI
    return convert_encoding(strUTF8, "UTF-8", "GBK");
}

std::string RbsLib::Encoding::CharsetConvert::ANSItoUTF8(const std::string& strAnsi)
{
	return strAnsi;
    return convert_encoding(strAnsi, "GBK", "UTF-8");
}
#endif
