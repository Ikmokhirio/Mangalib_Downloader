#ifndef DAEDALUS_MANGASEARCHER_H
#define DAEDALUS_MANGASEARCHER_H

#include "Images/ImageDescriptor.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "UriParser.h"
#include <format>
#include <fstream>
#include <httplib.h>
#include <iostream>

struct Manga {
  uint64_t id;
  std::string mangaName;
  std::string originalName;
  std::string russianName;
  std::string englishName;
  std::string summary;
  std::string cover;
  std::string thumbnail;
  std::string thumbnailBinary;

  std::string link;

  ImageDescriptor* img = nullptr;
  std::string thumbnailPath;

  bool selected;
  bool empty;

  Manga()
  {
    empty = true;
    img = nullptr;
  }
};

class MangaSearcher {
private:
  httplib::Client cli;
  httplib::Client thumbCli;

  std::vector<Manga> manga;

public:
  MangaSearcher(Uri uri, std::string cookie);

  void FindManga(const std::string& name, bool hires = false);

  inline std::vector<Manga>& List() { return manga; }

  inline void Clear()
  {
    for(auto& m: manga) {
      if(m.img) {
        m.img->Clear();// Delete images
      }
    }
    manga = std::vector<Manga>();
  }
};

#endif// DAEDALUS_MANGASEARCHER_H
