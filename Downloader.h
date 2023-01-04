#ifndef MANGALIB_DOWNLOADER_DOWNLOADER_H
#define MANGALIB_DOWNLOADER_DOWNLOADER_H

#include "Logger.h"
#include "nlohmann/json_fwd.hpp"
#include <chrono>
#include <filesystem>
#include <stdexcept>
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "Combiner.h"
#include "HtmlCombiner.h"
#include "UriParser.h"
#include <Daedalus.h>
#include <cstdio>
#include <format>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string_view>
#include <thread>

struct Chapter {
  std::string chapterId;
  int volumeNumber;
  int chapterNumber;

  bool selected;
  bool finished;
  bool errorOnLastOperation;

  Chapter(const std::string& id, const int& v, const int& n)
  {
    chapterId = id;
    volumeNumber = v;
    chapterNumber = n;
    selected = false;
    errorOnLastOperation = false;
    finished = false;
  }
};

class MangalibDownloaderError : public std::runtime_error {
public:
  MangalibDownloaderError(std::string errorMessage)
      : std::runtime_error(errorMessage)
  {
    DS_ERROR(errorMessage);
  }
};

template<typename T>
T TryGetValue(nlohmann::json data, const std::string& field)
{
  if(!data.contains(field)) {
    throw MangalibDownloaderError(std::format("Cannot find {0}", field));
  }

  return data[field].get<T>();
}

class Downloader {
private:
  const std::string VOLUME = "chapter_volume";
  const std::string CHAPTER_NUMBER = "chapter_number";
  const std::string CHAPTER = "chapter";
  const std::string CHAPTER_ID = "chapter_id";
  const std::string SLUG = "slug";
  const std::string IMAGES = "images";
  const std::string DOWNLOAD_SERVER = "downloadServer";
  const std::string MANGALIB_URL = "https://mangalib.me";

  Uri uri;
  std::string jsonData;
  std::string mangaName;

  int maxAttempt;
  int errorSleepTime;
  int requestDelayMs;

  httplib::Client cli;

  nlohmann::json currentChapter;
  nlohmann::json chaptersList;

  std::vector<Chapter> chapters;
  std::vector<Combiner*> combiners;

  void ExtractJsonData();

  void ProcessCurrentChapter();

  void ExtractChaptersList();

public:
  Downloader(Uri url, std::string cookie, std::vector<Combiner*> combs, int requestDelay = 0, int errorDelay = 0, int maxAttemptCount = 3);

  std::vector<Chapter> GetChapters();

  void DownloadChapter(Chapter chapter);
};

#endif// MANGALIB_DOWNLOADER_DOWNLOADER_H
