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
  int branchNumber;

  bool selected;
  bool finished;
  bool errorOnLastOperation;

  Chapter(const std::string& id, const int& v, const int& n, const int branch = 0)
  {
    chapterId = id;
    volumeNumber = v;
    chapterNumber = n;
    branchNumber = branch;

    selected = false;
    errorOnLastOperation = false;
    finished = false;
  }
};

struct Team {
  std::string name;
  int branch;

  Team(const std::string& teamName, const int& branchNumber)
  {
    branch = branchNumber;
    name = teamName;
  }
};

class MangalibDownloaderError : public std::runtime_error {
public:
  MangalibDownloaderError(std::string errorMessage)
      : std::runtime_error(errorMessage)
  {
    DS_ERROR(errorMessage.c_str());
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
  const std::string ENG_NAME = "engName";
  const std::string RUS_NAME = "rusName";
  const std::string ORIG_NAME = "name";

  const std::string VOLUME = "chapter_volume";
  const std::string CHAPTER_NUMBER = "chapter_number";
  const std::string CHAPTER = "chapter";
  const std::string CHAPTER_ID = "chapter_id";
  const std::string SLUG = "slug";
  const std::string IMAGES = "images";
  const std::string DOWNLOAD_SERVER = "downloadServer";
  const std::string MANGALIB_URL = "https://mangalib.me";
  const std::string BRANCH_ID = "branch_id";

  Uri uri;
  std::string jsonData;
  std::wstring mangaName;
  std::wstring originalName;
  std::wstring russianName;
  std::wstring englishName;

  int branchId;

  int maxAttempt;
  int errorSleepTime;
  int requestDelayMs;

  httplib::Client cli;

  nlohmann::json currentChapter;
  nlohmann::json chaptersList;
  nlohmann::json branchesList;

  std::vector<Chapter> chapters;
  std::vector<Combiner*> combiners;

  void ProcessCurrentChapter();

  void ExtractChaptersList();

public:
  Downloader(Uri url, std::string cookie, std::vector<Combiner*> combs, int requestDelay = 0, int errorDelay = 0, int maxAttemptCount = 3);

  std::vector<Team> GetTeams();

  std::vector<Chapter> GetChapters();

  void DownloadChapter(Chapter chapter);

  void ExtractJsonData();

  inline void SelectBranch(const int& branch)
  {
    DS_INFO("Выбор ветки {0}", branch);
    branchId = branch;
  }

  inline void SelectName(std::wstring name) { mangaName = name; }

  inline std::wstring GetRussianName() { return russianName; }

  inline std::wstring GetEnglishName() { return englishName; }

  inline std::wstring GetOriginalName() { return originalName; }
};

#endif// MANGALIB_DOWNLOADER_DOWNLOADER_H
