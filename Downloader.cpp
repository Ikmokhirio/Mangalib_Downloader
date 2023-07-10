#include "Downloader.h"
#include "Logger.h"
#include "httplib.h"
#include <Utils/Utils.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

Downloader::Downloader(Uri url, std::string c, int requestDelay, int errorDelay, int maxAttemptCount)
    : uri(url)
    , branchId(0)
    , maxAttempt(maxAttemptCount)
    , errorSleepTime(errorDelay)
    , requestDelayMs(requestDelay)
    , cli(url.ProtocolHost)
{

  DS_INFO("Установленные настройки: кол-во попыток={0}, пауза при ошибке={1}с., пауза между запросами={2}мс.", maxAttempt, errorSleepTime, requestDelayMs);

  cookie = c;
  cli.set_default_headers({{"Cookie", std::format("mangalib_session={0}", cookie)}, {"Origin", "https://mangalib.me"}, {"Referer", "https://mangalib.me/"}});

  cli.set_connection_timeout(std::chrono::seconds(5));
  cli.set_read_timeout(std::chrono::seconds(5));
  cli.set_write_timeout(std::chrono::seconds(5));
}

std::string ExtractCookie(std::string header, std::string name)
{
  if(header.empty() || name.empty()) {
    return {};
  }
  std::string marker = std::format("{0}=", name);
  int start = header.find(marker);
  int end = header.find(";");
  // DS_INFO("HEADER_SIZE : {0}, start : {1}, end: {2}", header.size(), start, end);

  return header.substr(start + marker.size(), end - start - marker.size());
}

std::wstring ExecutablePath()
{
  WCHAR buffer[MAX_PATH] = {0};
  GetModuleFileNameW(NULL, buffer, MAX_PATH);
  std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
  return std::wstring(buffer).substr(0, pos);
}

void Downloader::DownloadChapter(Chapter chapter)
{
  std::wstring path = ExecutablePath();
  path.append(L"\\");
  path.append(Converter::ToWString(currentManga.mangaName));
  CreateDirectoryWithChecking(path.c_str());

  DS_INFO("Цель : Том {0} глава {1}", chapter.volumeNumber, chapter.chapterNumber);
  auto res = cli.Get(std::format("/download/{0}", chapter.chapterId));
  xsrfToken = ExtractCookie(res->get_header_value(SET_COOKIE, 0), "XSRF-TOKEN");
  // DS_INFO("XSRF-TOKEN : {0}", xsrfToken);

  httplib::Headers headers = {
      {"Accept-Encoding", "gzip, deflate"},
      {"Cookie", std::format("mangalib_session={0};XSRF-TOKEN={1}", cookie, xsrfToken)},
      {"Origin", "https://mangalib.me"},
      {"Referer", "https://mangalib.me/"},
      {"Sec-Fetch-Dest", "empty"},
      {"Sec-Fetch-Mode", "cors"},
      {"Sec-Fetch-Site", "cross-site"}};

  auto downloadData = res->body;
  DS_INFO("Получение ссылок на картинки");
  auto downloadDataJson = nlohmann::json::parse(downloadData);

  std::string server = TryGetValue<std::string>(downloadDataJson, DOWNLOAD_SERVER);
  httplib::Client pictureDownloader(server);
  pictureDownloader.set_connection_timeout(std::chrono::seconds(5));
  pictureDownloader.set_read_timeout(std::chrono::seconds(5));
  pictureDownloader.set_write_timeout(std::chrono::seconds(5));
  //DS_DEBUG("Сервер для скачки : {0}", server);

  auto images = TryGetValue<nlohmann::json>(downloadDataJson, IMAGES);
  auto chapterStruct = TryGetValue<nlohmann::json>(downloadDataJson, CHAPTER);
  std::string chapterId = TryGetValue<std::string>(chapterStruct, SLUG);
  int attemptCount = 0;

  int listNumber = 1;
  for(auto& img: images) {
    attemptCount = 0;

    while(attemptCount < maxAttempt) {
      attemptCount++;

      httplib::Result file = pictureDownloader.Get(std::format("{0}/manga{1}/chapters/{2}/{3}", server, uri.Path, chapterId, img.get<std::string>()), headers);

      if(!file) {
        DS_ERROR("Сервер ничего не вернул");
        DS_ERROR("Повторная попытка после паузы в 250мс");
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        continue;
      }
      if(file.error() != httplib::Error::Success) {
        DS_ERROR("Ошибка при загрузке : {0}", httplib::to_string(file.error()));
        DS_ERROR("Повторная попытка после паузы в {0}с", errorSleepTime);
        std::this_thread::sleep_for(std::chrono::seconds(errorSleepTime));
        continue;
      }
      if(file->status != 200) {
        DS_ERROR("Ошибка {0} | Тело ответа : {1}", file->status, file->body);
        DS_ERROR("Повторная попытка после паузы в {0}с", errorSleepTime);
        std::this_thread::sleep_for(std::chrono::seconds(errorSleepTime));
        continue;
      }
      antiDDOSToken = ExtractCookie(file->get_header_value(SET_COOKIE, 0), "__ddg1_");
      if(!antiDDOSToken.empty()) {// New token
        headers = {
            {"Accept-Encoding", "gzip, deflate"},
            {"Cookie", std::format("mangalib_session={0};XSRF-TOKEN={1};__ddg1_={2}", cookie, xsrfToken, antiDDOSToken)},
            {"Origin", "https://mangalib.me"},
            {"Referer", "https://mangalib.me/"},
            {"Sec-Fetch-Dest", "empty"},
            {"Sec-Fetch-Mode", "cors"},
            {"Sec-Fetch-Site", "cross-site"}};
      }

      std::wostringstream ss;
      //ss << "./" << mangaName << "/" << Converter::ToWString(img.get<std::string>());
      std::filesystem::path downloadFile = img.get<std::string>();
      ss << "./" << Converter::ToWString(currentManga.mangaName) << "/vol" << chapter.volumeNumber << "_ch" << chapter.chapterNumber << "_p" << listNumber << Converter::ToWString(downloadFile.extension().string());
      //DS_INFO("{0}", Converter::ToString(ss.str()));
      for(auto combiner: combiners) {
        combiner->AddFile(file->body, ss.str());
      }
      break;
    }
    if(attemptCount >= maxAttempt) {
      throw MangalibDownloaderError(std::format("Не получилось загрузить после {0} попыток", maxAttempt));
    }
    listNumber++;
  }

  std::wostringstream ss;
  std::wstring outputPath;
  ss << "./" << Converter::ToWString(currentManga.mangaName) << "/vol_" << chapter.volumeNumber << "_ch_" << chapter.chapterNumber;
  //std::string outputPath = std::format("./{0}/vol_{1}_ch_{2}", mangaName, chapter.volumeNumber, chapter.chapterNumber);
  std::string previousChapter = std::format("vol_{0}_ch_{1}", chapter.volumeNumber, chapter.chapterNumber - 1);
  std::string nextChapter = std::format("vol_{0}_ch_{1}", chapter.volumeNumber, chapter.chapterNumber + 1);

  for(auto combiner: combiners) {
    combiner->SaveTo(ss.str(), previousChapter, nextChapter);
  }
  DS_INFO("Успешно том {0} глава {1}", chapter.volumeNumber, chapter.chapterNumber);
}

std::vector<Chapter> Downloader::GetChapters()
{
  chapters = std::vector<Chapter>();
  if(jsonData.empty()) {
    throw MangalibDownloaderError("Нет json данных");
  }

  // Parsing
  DS_INFO("Извлечение информации о главах");
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
    throw MangalibDownloaderError("Нет json данных");
  }

  std::vector<Team> teams;

  auto res = nlohmann::json::parse(jsonData);

  const std::string MANGA_BLOCK = "manga";
  const std::string CHAPTERS = "chapters";
  const std::string BRANCHES = "branches";
  const std::string TEAMS = "teams";
  const std::string NAME = "name";

  if(!res.contains(MANGA_BLOCK)) {
    throw MangalibDownloaderError("Не удалось найти блок \"manga\"");
  }

  if(!res.contains(CHAPTERS)) {
    throw MangalibDownloaderError("Не удалось найти блок \"chapters\"");
  }

  if(!res[CHAPTERS].contains(BRANCHES)) {
    throw MangalibDownloaderError("Не удалось найти блок \"branches\"");
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
    throw MangalibDownloaderError("Не удалось найти блок \"manga\"");
  }

  if(!res.contains(CHAPTERS)) {
    throw MangalibDownloaderError("Не удалось найти блок \"chapters\"");
  }

  if(!res[CHAPTERS].contains(LIST)) {
    throw MangalibDownloaderError("Не удалось найти блок \"list\"");
  }

  chaptersList = res[CHAPTERS][LIST];

  auto manga = res[MANGA_BLOCK];

  if(!manga.contains(ENG_NAME)) {
    throw MangalibDownloaderError("Нет имени у произведения");
  }

  currentManga.mangaName = manga[ENG_NAME];

  currentManga.originalName = manga[ORIG_NAME];
  currentManga.englishName = manga[ENG_NAME];
  currentManga.russianName = manga[RUS_NAME];
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
  DS_INFO("Читаем json");

  auto res = cli.Get(uri.Path);
  if(res.error() != httplib::Error::Success) {
    throw MangalibDownloaderError(std::format("Ошибка при запросе : {0}", httplib::to_string(res.error())));
  }

  const std::string MARKER_START = "window.__DATA__ = ";
  const std::string MARKER_END = "window._SITE_COLOR_";// TODO : Script tag works better

  std::string body = res->body;

  int start = body.find(MARKER_START);
  if(start == body.npos) {
    throw MangalibDownloaderError("Некорректный формат ответа");
  }

  int end = body.find(MARKER_END);
  if(end == body.npos) {
    throw MangalibDownloaderError("Некорректный формат ответа");
  }

  jsonData = body.substr(start + MARKER_START.size(), end - (start + MARKER_END.size()));
  while(!jsonData.ends_with(';')) {
    jsonData.pop_back();
  }
  jsonData.pop_back();
}
