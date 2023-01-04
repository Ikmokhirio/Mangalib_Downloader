#ifndef MANGALIB_DOWNLOADER_HTMLCOMBINER_H
#define MANGALIB_DOWNLOADER_HTMLCOMBINER_H

#include "Combiner.h"
#include <Base64.hpp>
#include <format>
#include <fstream>
#include <string>
#include <vector>

class HtmlCombiner : public Combiner
{
private:
  std::string original;
  const std::string START_STYLE = R"(
<!DOCTYPE html>
<html>
<body>
  <style>)";

  const std::string END_STYLE = R"(
    .buttons {
      display: flex;
      flex-direction: row;
      justify-content: center;
      align-items: center;
    }
    .buttons a {
      margin: 48px;
    }
    .content {
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
    }
  </style>

  <div class="content">)";
  // IMAGES WITH <img>
  const std::string MID = R"(
     </div>
  <div class="buttons">)";
  // <a href="/">PREV</a>
  // <a href="/">NEXT</a>
  const std::string END = R"(
      </div>
</body>
</html>)";
  std::vector<std::string> files;

  std::string result;

public:
  HtmlCombiner(int width);

  void AddFile(const std::string& file, const std::string& name = "") override;

  void SaveTo(const std::string& path, const std::string& prev, const std::string& next) override;
};

#endif// MANGALIB_DOWNLOADER_HTMLCOMBINER_H
