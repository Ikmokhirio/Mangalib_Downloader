#include "Converter.h"

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Converter::converter;

std::wstring Converter::ToWString(const std::string& src)
{
  return converter.from_bytes(src);
}

std::string Converter::ToString(const std::wstring& src)
{
  return converter.to_bytes(src);
}
