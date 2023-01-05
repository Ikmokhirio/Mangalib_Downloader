#include "Downloader.h"
#include "httplib.h"

Downloader::Downloader(Uri url, std::string cookie, std::vector<Combiner*> combs, int requestDelay, int errorDelay, int maxAttemptCount)
    : uri(url)
    , maxAttempt(maxAttemptCount)
    , errorSleepTime(errorDelay)
    , requestDelayMs(requestDelay)
    , cli(MANGALIB_URL)
    , combiners(combs)
{
  cli.set_default_headers({{"Cookie", std::format("mangalib_session={0}", cookie)}});
}

void Downloader::DownloadChapter(Chapter chapter)
{
  DS_DEBUG("Getting info for volume {0} chapter {1}", chapter.volumeNumber, chapter.chapterNumber);
  auto downloadData = cli.Get(std::format("/download/{0}", chapter.chapterId))->body;

  DS_DEBUG("Extracting picture urls");
  auto downloadDataJson = nlohmann::json::parse(downloadData);

  std::string server = TryGetValue<std::string>(downloadDataJson, DOWNLOAD_SERVER);
  httplib::Client pictureDownloader(server);
  DS_DEBUG("Download server : {0}", server);

  auto images = TryGetValue<nlohmann::json>(downloadDataJson, IMAGES);
  auto chapterStruct = TryGetValue<nlohmann::json>(downloadDataJson, CHAPTER);
  std::string chapterId = TryGetValue<std::string>(chapterStruct, SLUG);
  int attemptCount = 0;

  for(auto& img: images) {
    attemptCount = 0;

    while(attemptCount <= maxAttempt) {
      attemptCount++;

      httplib::Result file = pictureDownloader.Get(std::format("{0}/manga{1}/chapters/{2}/{3}", server, uri.Path, chapterId, img.get<std::string>()));

      if(file.error() != httplib::Error::Success) {
        DS_ERROR("An error occured on request : {0}", httplib::to_string(file.error()));
        DS_ERROR("Retrying");
        std::this_thread::sleep_for(std::chrono::seconds(errorSleepTime));
        continue;
      }
      if(!file) {
        DS_ERROR("Get return null");
        DS_ERROR("Retrying");
        std::this_thread::sleep_for(std::chrono::seconds(errorSleepTime));
        continue;
      }
      if(file->status != 200) {
        DS_ERROR("Error {0} | BODY : {1}", file->status, file->body);
        DS_ERROR("Retrying");
        std::this_thread::sleep_for(std::chrono::seconds(errorSleepTime));
        continue;
      }

      for(auto combiner: combiners) {
        combiner->AddFile(file->body);
      }
      break;
    }
    if(attemptCount >= maxAttempt) {
      throw MangalibDownloaderError(std::format("Could not load chapter after {0} attempts", maxAttempt));
    }
  }

  std::string outputPath = std::format("./{0}/vol_{1}_ch_{2}", mangaName, chapter.volumeNumber, chapter.chapterNumber);
  std::string previousChapter = std::format("vol_{0}_ch_{1}", chapter.volumeNumber, chapter.chapterNumber - 1);
  std::string nextChapter = std::format("vol_{0}_ch_{1}", chapter.volumeNumber, chapter.chapterNumber + 1);

  for(auto combiner: combiners) {
    combiner->SaveTo(outputPath, previousChapter, nextChapter);
  }
  DS_INFO("Finished {0} vol{1}. ch{2}", mangaName, chapter.volumeNumber, chapter.chapterNumber);
}

std::vector<Chapter> Downloader::GetChapters()
{
  // Getting info about amount of chapters and volumes
  DS_DEBUG("Extracting json data");
  ExtractJsonData();

  // Parsing
  DS_DEBUG("Extracting chapters data");
  ExtractChaptersList();

  DS_DEBUG("Manga name is \"{0}\"", mangaName);
  DS_DEBUG("Creating folder...");
  std::filesystem::create_directory(mangaName);

  for(auto it = chaptersList.rbegin(); it != chaptersList.rend(); ++it) {
    currentChapter = it.value();
    ProcessCurrentChapter();
  }

  return chapters;
}

void Downloader::ExtractChaptersList()
{
  auto res = nlohmann::json::parse(jsonData);

  const std::string MANGA_BLOCK = "manga";
  const std::string ENG_NAME = "engName";
  const std::string CHAPTERS = "chapters";
  const std::string LIST = "list";

  if(!res.contains(MANGA_BLOCK)) {
    throw MangalibDownloaderError("Cannot find \"manga\" block in data");
  }

  if(!res.contains(CHAPTERS)) {
    throw MangalibDownloaderError("Cannot find \"chapters\" block in data");
  }

  if(!res[CHAPTERS].contains(LIST)) {
    throw MangalibDownloaderError("Cannot find \"list\" block in data");
  }

  chaptersList = res[CHAPTERS][LIST];

  auto manga = res[MANGA_BLOCK];

  if(!manga.contains(ENG_NAME)) {
    throw MangalibDownloaderError("Cannot find english name for manga");
  }

  mangaName = manga[ENG_NAME];
}

void Downloader::ProcessCurrentChapter()
{
  int volumeNumber = TryGetValue<int>(currentChapter, VOLUME);
  int chapterNumber = std::stoi(TryGetValue<std::string>(currentChapter, CHAPTER_NUMBER));
  std::string chapterId = std::to_string(TryGetValue<int>(currentChapter, CHAPTER_ID));

  chapters.emplace_back(Chapter(chapterId, volumeNumber, chapterNumber));
}

void Downloader::ExtractJsonData()
{
  auto res = cli.Get(uri.Path);
  if(res.error() != httplib::Error::Success) {
    throw MangalibDownloaderError(std::format("An error occured on request : {0}", httplib::to_string(res.error())));
  }

  const std::string MARKER_START = "window.__DATA__ = ";
  const std::string MARKER_END = "window._SITE_COLOR_";

  std::string body = res->body;
  int start = body.find(MARKER_START);
  if(start == body.npos) {
    throw MangalibDownloaderError("Cannot find data in response body");
  }

  int end = body.find(MARKER_END);
  if(end == body.npos) {
    throw MangalibDownloaderError("Cannot find data in response body");
  }

  jsonData = body.substr(start + MARKER_START.size(), end - (start + MARKER_END.size()));
  while(!jsonData.ends_with(';')) {
    jsonData.pop_back();
  }
  jsonData.pop_back();
}
