#ifndef MANGALIBDOWNLOADER_CONVERTER_H
#define MANGALIBDOWNLOADER_CONVERTER_H

#include <codecvt>
#include <locale>
#include <string>

class Converter {
private:
  static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

public:
  static std::wstring ToWString(const std::string& src);

  static std::string ToString(const std::wstring& src);
};

#endif// MANGALIBDOWNLOADER_CONVERTER_H
