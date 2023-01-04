#include "HtmlCombiner.h"
#include <Daedalus.h>
#include <format>

HtmlCombiner::HtmlCombiner(int width)
{
  result = START_STYLE;
  result.append(std::format("img {{ width: {0}vw; }}", width));
  result.append(END_STYLE);
  original = result;
}

void HtmlCombiner::AddFile(const std::string& file, const std::string& name)
{
  auto res = Base64::to(file);
  result.append(std::format("<img src=\"data:image;base64,{0}\">\n", res));
}

void HtmlCombiner::SaveTo(const std::string& path, const std::string& prev, const std::string& next)
{
  DS_DEBUG("Saving html document as {0}.html", path);
  result.append(MID);
  result.append(std::format("<a href=\"{0}.html\">PREV</a>", prev));
  result.append(std::format("<a href=\"{0}.html\">NEXT</a>", next));
  result.append(END);

  std::ofstream out;
  out.open(std::format("{0}.html", path));
  out << result;
  out.close();

  result = original;
}

