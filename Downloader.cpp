#include "Downloader.h"
#include "Converter.h"
#include "httplib.h"
#include <sstream>

std::string GetLastErrorAsString()
{
  //Get the error message ID, if any.
  DWORD errorMessageID = ::GetLastError();
  if(errorMessageID == 0) {
    return std::string();//No error message has been recorded
  }

  LPWSTR messageBuffer = nullptr;

  size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &messageBuffer, 0, NULL);

  //Copy the error message into a std::string.
  std::wstring message(messageBuffer, size);

  //Free the Win32's string's buffer.
  LocalFree(messageBuffer);

  return Converter::ToString(message);
}

void CreateDirectoryWithChecking(const wchar_t* i_LPCWSTR_FolderPath)
{
  ::WIN32_FIND_DATAW data;
  ::HANDLE handle = ::FindFirstFileW(i_LPCWSTR_FolderPath, &data);
  if(handle == INVALID_HANDLE_VALUE) {
    auto result = ::CreateDirectoryW(i_LPCWSTR_FolderPath, NULL);
    if(result == 0) {
      DS_INFO("Ошибка код {0} : {1}", GetLastError(), GetLastErrorAsString());
    }
  } else {
    FindClose(handle);
  }
}

Downloader::Downloader(Uri url, std::string cookie, std::vector<Combiner*> combs, int requestDelay, int errorDelay, int maxAttemptCount)
    : uri(url)
    , branchId(0)
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
  CreateDirectoryWithChecking(mangaName.c_str());

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

      std::wostringstream ss;
      ss << "./" << mangaName << "/" << Converter::ToWString(img.get<std::string>());
      for(auto combiner: combiners) {
        combiner->AddFile(file->body, ss.str());
      }
      break;
    }
    if(attemptCount >= maxAttempt) {
      throw MangalibDownloaderError(std::format("Could not load chapter after {0} attempts", maxAttempt));
    }
  }

  std::wostringstream ss;
  std::wstring outputPath;
  ss << "./" << mangaName << "/vol_" << chapter.volumeNumber << "_ch_" << chapter.chapterNumber;
  //std::string outputPath = std::format("./{0}/vol_{1}_ch_{2}", mangaName, chapter.volumeNumber, chapter.chapterNumber);
  std::string previousChapter = std::format("vol_{0}_ch_{1}", chapter.volumeNumber, chapter.chapterNumber - 1);
  std::string nextChapter = std::format("vol_{0}_ch_{1}", chapter.volumeNumber, chapter.chapterNumber + 1);

  for(auto combiner: combiners) {
    combiner->SaveTo(ss.str(), previousChapter, nextChapter);
  }
  DS_INFO("Finished vol{0}. ch{1}", chapter.volumeNumber, chapter.chapterNumber);
}

std::vector<Chapter> Downloader::GetChapters()
{
  chapters = std::vector<Chapter>();
  if(jsonData.empty()) {
    throw MangalibDownloaderError("No json data [call ExtractJsonData() first]");
  }

  // Parsing
  DS_DEBUG("Extracting chapters data");
  ExtractChaptersList();

  // Figure wstring
  //DS_DEBUG("Manga name is \"{0}\"", mangaName);

  for(auto it = chaptersList.rbegin(); it != chaptersList.rend(); ++it) {
    currentChapter = it.value();
    ProcessCurrentChapter();
  }

  return chapters;
}

std::vector<Team> Downloader::GetTeams()
{
  if(jsonData.empty()) {
    throw MangalibDownloaderError("No json data [call ExtractJsonData() first]");
  }

  std::vector<Team> teams;

  auto res = nlohmann::json::parse(jsonData);

  const std::string MANGA_BLOCK = "manga";
  const std::string CHAPTERS = "chapters";
  const std::string BRANCHES = "branches";
  const std::string TEAMS = "teams";
  const std::string NAME = "name";

  if(!res.contains(MANGA_BLOCK)) {
    throw MangalibDownloaderError("Cannot find \"manga\" block in data");
  }

  if(!res.contains(CHAPTERS)) {
    throw MangalibDownloaderError("Cannot find \"chapters\" block in data");
  }

  if(!res[CHAPTERS].contains(BRANCHES)) {
    throw MangalibDownloaderError("Cannot find \"branches\" block in data");
  }

  branchesList = res[CHAPTERS][BRANCHES];

  for(auto& branch: branchesList) {
    std::stringstream ss;
    for(auto& team: branch[TEAMS]) {
      ss << team[NAME] << " ";
    }
    teams.push_back(Team(ss.str(), branch["id"]));
  }

  return teams;
}

void Downloader::ExtractChaptersList()
{
  auto res = nlohmann::json::parse(jsonData);

  const std::string MANGA_BLOCK = "manga";
  const std::string CHAPTERS = "chapters";
  const std::string BRANCHES = "branches";
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

  mangaName = Converter::ToWString(manga[ENG_NAME]);

  originalName = Converter::ToWString(manga[ORIG_NAME]);
  englishName = Converter::ToWString(manga[ENG_NAME]);
  russianName = Converter::ToWString(manga[RUS_NAME]);
}

void Downloader::ProcessCurrentChapter()
{
  int volumeNumber = TryGetValue<int>(currentChapter, VOLUME);
  int chapterNumber = std::stoi(TryGetValue<std::string>(currentChapter, CHAPTER_NUMBER));
  std::string chapterId = std::to_string(TryGetValue<int>(currentChapter, CHAPTER_ID));
  if(branchId != 0) {
    try {
      int currentBranch = TryGetValue<int>(currentChapter, BRANCH_ID);
      if(currentBranch != branchId) {
        return;
      }
    } catch(MangalibDownloaderError& e) {
      DS_INFO(e.what());
      return;
    }
  }

  chapters.emplace_back(Chapter(chapterId, volumeNumber, chapterNumber, branchId));
}

void Downloader::ExtractJsonData()
{
  // Getting info about amount of chapters and volumes
  DS_DEBUG("Extracting json data");

  auto res = cli.Get(uri.Path);
  if(res.error() != httplib::Error::Success) {
    throw MangalibDownloaderError(std::format("An error occured on request : {0}", httplib::to_string(res.error())));
  }

  const std::string MARKER_START = "window.__DATA__ = ";
  const std::string MARKER_END = "window._SITE_COLOR_"; // TODO : Script tag works better

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
