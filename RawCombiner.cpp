#include "RawCombiner.h"

RawCombiner::RawCombiner() {}

void RawCombiner::AddFile(const std::string& file, const std::string& name)
{
  std::ofstream out;
  out.open(name, std::ios::binary);
  out << file;
  out.close();
}

void RawCombiner::SaveTo(const std::string& path, const std::string& prev, const std::string& next)
{
}

