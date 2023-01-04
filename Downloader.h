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

class MangalibDownloaderError : public std::runtime_error
{
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

class Downloader
{
private:
  Uri uri;
  std::string jsonData;
  std::string mangaName;

  int errorDelayS;
  int requestDelayMs;

  const std::string MANGALIB_URL = "https://mangalib.me";
  httplib::Client cli;

  nlohmann::json currentChapter;
  nlohmann::json chaptersList;
  std::vector<Combiner*> combiners;

  void ExtractJsonData()
  {
    auto res = cli.Get(uri.Path);
    const std::string MARKER_START = "window.__DATA__ = ";
    const std::string MARKER_END = "window._SITE_COLOR_";

    std::string body = res->body;
    int start = body.find(MARKER_START);
    int end = body.find(MARKER_END);

    jsonData = body.substr(start + MARKER_START.size(), end - (start + MARKER_END.size()));
    while(!jsonData.ends_with(';')) {
      jsonData.pop_back();
    }
    jsonData.pop_back();
  }

  void ProcessCurrentChapter()
  {
    const std::string VOLUME = "chapter_volume";
    const std::string CHAPTER_NUMBER = "chapter_number";
    const std::string CHAPTER = "chapter";
    const std::string CHAPTER_ID = "chapter_id";
    const std::string SLUG = "slug";
    const std::string IMAGES = "images";
    const std::string DOWNLOAD_SERVER = "downloadServer";

    int volumeNumber = TryGetValue<int>(currentChapter, VOLUME);

    int chapterNumber = std::stoi(TryGetValue<std::string>(currentChapter, CHAPTER_NUMBER));

    std::string chapterId = std::to_string(TryGetValue<int>(currentChapter, CHAPTER_ID));

    DS_INFO("Getting info for volume {0} chapter {1}", volumeNumber, chapterNumber);
    auto downloadData = cli.Get(std::format("/download/{0}", chapterId))->body;

    DS_DEBUG("Extracting picture urls");
    auto downloadDataJson = nlohmann::json::parse(downloadData);

    std::string server = TryGetValue<std::string>(downloadDataJson, DOWNLOAD_SERVER);

    DS_DEBUG("Download server : {0}", server);
    httplib::Client pictureDownloader(server);

    auto images = TryGetValue<nlohmann::json>(downloadDataJson, IMAGES);
    auto chapterStruct = TryGetValue<nlohmann::json>(downloadDataJson, CHAPTER);
    chapterId = TryGetValue<std::string>(chapterStruct, SLUG);
    int attemptCount = 0;

    for(auto& img: images) {
      attemptCount = 0;

      while(attemptCount <= 3) {
        attemptCount++;

        httplib::Result file = pictureDownloader.Get(std::format("{0}/manga{1}/chapters/{2}/{3}", server, uri.Path, chapterId, img.get<std::string>()));

        if(file.error() != httplib::Error::Success) {
          DS_ERROR("An error occured on request : {0}", httplib::to_string(file.error()));
          std::this_thread::sleep_for(std::chrono::seconds(errorDelayS));
          continue;
        }
        if(!file) {
          DS_ERROR("Get return null");
          std::this_thread::sleep_for(std::chrono::seconds(errorDelayS));
          continue;
        }
        if(file->status != 200) {
          DS_ERROR("Error {0} | BODY : {1}", file->status, file->body);
          std::this_thread::sleep_for(std::chrono::seconds(errorDelayS));
          continue;
        }

        for(auto combiner: combiners) {
          combiner->AddFile(file->body);
        }
        break;
      }
      if(attemptCount >= 3) {
        throw MangalibDownloaderError("Could not load after 3 attempts");
      }
    }

    std::string outputPath = std::format("./{0}/vol_{1}_ch_{2}", mangaName, volumeNumber, chapterNumber);
    std::string prevFile = std::format("vol_{0}_ch_{1}", volumeNumber, chapterNumber - 1);
    std::string nextFile = std::format("vol_{0}_ch_{1}", volumeNumber, chapterNumber + 1);
    for(auto combiner: combiners) {
      combiner->SaveTo(outputPath, prevFile, nextFile);
    }
  }

  void
  ExtractChaptersList()
  {
    auto res = nlohmann::json::parse(jsonData);

    const std::string MANGA_BLOCK = "manga";
    const std::string ENG_NAME = "engName";
    const std::string CHAPTERS = "chapters";
    const std::string LIST = "list";

    if(!res.contains(MANGA_BLOCK)) {
      DS_ERROR("Cannot find \"manga\" block in data");
      return;
    }
    if(!res.contains(CHAPTERS)) {
      DS_ERROR("Cannot find \"chapters\" block in data");
      return;
    }
    if(!res[CHAPTERS].contains(LIST)) {
      DS_ERROR("Cannot find \"list\" block in data");
      return;
    }
    chaptersList = res[CHAPTERS][LIST];

    auto manga = res[MANGA_BLOCK];
    if(!manga.contains(ENG_NAME)) {
      DS_ERROR("Cannot find english name for manga");
      return;
    }
    mangaName = manga[ENG_NAME];
  }

public:
  Downloader(Uri url, std::string cookie, std::vector<Combiner*> combs, int requestDelay = 0, int errorDelay = 0)
      : uri(url)
      , errorDelayS(errorDelay)
      , requestDelayMs(requestDelay)
      , cli(MANGALIB_URL)
      , combiners(combs)
  {
    cli.set_default_headers({{"Cookie", std::format("mangalib_session={0}", cookie)}});
  }

  void Download()
  {
    DS_INFO("Extracting json data");
    ExtractJsonData();

    DS_INFO("Extracting chapters data");
    ExtractChaptersList();

    DS_INFO("Manga name is \"{0}\"", mangaName);
    DS_INFO("Creating folder...");
    std::filesystem::create_directory(mangaName);

    for(auto it = chaptersList.rbegin(); it != chaptersList.rend(); ++it) {
      currentChapter = it.value();
      ProcessCurrentChapter();
      std::this_thread::sleep_for(std::chrono::milliseconds(requestDelayMs));
    }
  }
};

#endif// MANGALIB_DOWNLOADER_DOWNLOADER_H
