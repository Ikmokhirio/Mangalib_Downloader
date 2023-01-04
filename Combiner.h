#ifndef MANGALIB_DOWNLOADER_COMBINER_H
#define MANGALIB_DOWNLOADER_COMBINER_H

#include <Base64.hpp>
#include <format>
#include <fstream>
#include <string>
#include <vector>

class Combiner
{
private:
public:
  Combiner() = default;

  virtual void AddFile(const std::string& file, const std::string &name = "") = 0;

  virtual void SaveTo(const std::string& path, const std::string& prev, const std::string& next) = 0;

};

#endif// MANGALIB_DOWNLOADER_COMBINER_H
