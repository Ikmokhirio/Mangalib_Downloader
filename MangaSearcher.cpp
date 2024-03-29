#include "MangaSearcher.h"
#include "Downloader.h"
#include <Utils/Utils.h>
#include <format>
#include <stdint.h>
#include <string>
#include <thread>

MangaSearcher::MangaSearcher(Uri uri, std::string cookie)
    : cli(uri.ProtocolHost)
    , thumbCli(uri.ProtocolHost)
    // , thumbCli("https://cover.imglib.info")
{
  cli.set_default_headers({{"Cookie", std::format("mangalib_session={0}", cookie)}, {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36"}});
  cli.set_connection_timeout(std::chrono::seconds(2));
  cli.set_read_timeout(std::chrono::seconds(2));
  cli.set_write_timeout(std::chrono::seconds(2));

  thumbCli.set_default_headers({{"Cookie", std::format("mangalib_session={0}", cookie)}, {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36"}, {"Connection", "keep-alive"}});
  thumbCli.set_connection_timeout(std::chrono::seconds(2));
  thumbCli.set_read_timeout(std::chrono::seconds(2));
  thumbCli.set_write_timeout(std::chrono::seconds(2));
  thumbCli.set_follow_location(true);// Redirect anti-dos
}

void MangaSearcher::FindManga(const std::string& name, bool hiRes)
{
  manga = std::vector<Manga>();

  auto res = cli.Get(std::format("/search?type=manga&q={0}", (name)));

  if(res.error() != httplib::Error::Success) {
    throw MangalibDownloaderError(std::format("Ошибка при запросе : {0}", httplib::to_string(res.error())));
  }

  auto jsonData = nlohmann::json::parse(res->body);

  for(auto& data: jsonData) {
    manga.emplace_back();

    manga.back().id = TryGetValue<uint64_t>(data, "id");
    manga.back().mangaName = TryGetValue<std::string>(data, "eng_name");

    manga.back().originalName = TryGetValue<std::string>(data, "name");
    manga.back().englishName = TryGetValue<std::string>(data, "eng_name");
    manga.back().russianName = TryGetValue<std::string>(data, "rus_name");

    manga.back().summary = TryGetValue<std::string>(data, "summary");

    manga.back().cover = TryGetValue<std::string>(data, "coverImage");
    manga.back().thumbnail = TryGetValue<std::string>(data, "coverImageThumbnail");

    manga.back().link = TryGetValue<std::string>(data, "href");
    // DS_INFO("{0}", Uri::Parse(manga.back().thumbnail).Path);

    // Download thumbnail
    if(hiRes) {
      res = thumbCli.Get(Uri::Parse(manga.back().cover).Path);
    } else {
      res = thumbCli.Get(Uri::Parse(manga.back().thumbnail).Path);
    }

    if(res.error() != httplib::Error::Success) {
      throw MangalibDownloaderError(std::format("Ошибка при запросе : {0}", httplib::to_string(res.error())));
    }

    if(manga.back().thumbnail.empty()) {
      return;
    }
    if(res->body.find("DOCTYPE") != std::string::npos) {
      manga.back().thumbnailBinary = std::string();
    } else {
      manga.back().thumbnailBinary = res->body;
    }

    manga.back().selected = false;
    manga.back().empty = false;
  }
}

