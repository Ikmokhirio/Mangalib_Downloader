#include "RawCombiner.h"
#include "Logger.h"
#include <Utils/Utils.h>

RawCombiner::RawCombiner() {}

void RawCombiner::AddFile(const std::string& file, const std::wstring& name)
{
  std::ofstream out;
  out.open(name, std::ios::binary);
  out << file;
  out.close();
}

void RawCombiner::SaveTo(const std::wstring& path, const std::string& prev, const std::string& next)
{
}

