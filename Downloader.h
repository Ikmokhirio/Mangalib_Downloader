#ifndef MANGALIB_DOWNLOADER_DOWNLOADER_H
#define MANGALIB_DOWNLOADER_DOWNLOADER_H

#include "Logger.h"
#include "nlohmann/json_fwd.hpp"
#include <chrono>
#include <filesystem>

#include "Error.h"

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

#include "MangaSearcher.h"

struct Chapter {
  std::string chapterId;
  int volumeNumber;
  int chapterNumber;
  int branchNumber;

  bool selected;
  bool finished;
  bool loading;
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
    loading = false;
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
  const std::string BRANCH_ID = "branch_id";
  const std::string SET_COOKIE = "set-cookie";

  Uri uri;
  std::string jsonData;

  int branchId;

  int maxAttempt;
  int errorSleepTime;
  int requestDelayMs;

  httplib::Client cli;

  std::string xsrfToken;
  std::string cookie;
  std::string antiDDOSToken;

  nlohmann::json currentChapter;
  nlohmann::json chaptersList;
  nlohmann::json branchesList;

  std::vector<Chapter> chapters;
  std::vector<Combiner*> combiners;

  void ProcessCurrentChapter();

  void ExtractChaptersList();

  Manga currentManga;

public:
  Downloader(Uri url, std::string cookie, int requestDelay = 0, int errorDelay = 0, int maxAttemptCount = 3);

  std::vector<Team> GetTeams();

  std::vector<Chapter> GetChapters();

  void DownloadChapter(Chapter chapter);

  void ExtractJsonData();

  inline void SelectBranch(const int& branch) { branchId = branch; }

  inline void SelectName(std::string name) { currentManga.mangaName = name; }

  inline Manga GetCurrentManga() { return currentManga; }

  inline void SetCombiners(std::vector<Combiner*> combs) { combiners = combs; }

  inline void SetMaxAttempts(int attempts) { maxAttempt = attempts; }

  inline void SetErrorDelay(int seconds) { errorSleepTime = seconds; }

  inline void SetRequestDelay(int milliseconds) { requestDelayMs = milliseconds; }
};

#endif// MANGALIB_DOWNLOADER_DOWNLOADER_H
