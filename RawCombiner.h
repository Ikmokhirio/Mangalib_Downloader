#ifndef MANGALIB_DOWNLOADER_RAWCOMBINER_H
#define MANGALIB_DOWNLOADER_RAWCOMBINER_H

#include "Combiner.h"

class RawCombiner : public Combiner
{
private:
public:
  RawCombiner();

  void AddFile(const std::string& file, const std::string &name) override;

  void SaveTo(const std::string& path, const std::string& prev, const std::string& next) override;
};

#endif// MANGALIB_DOWNLOADER_RAWCOMBINER_H
