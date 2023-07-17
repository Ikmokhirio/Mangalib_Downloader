#ifndef DAEDALUS_GUI_H
#define DAEDALUS_GUI_H

#include <future>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <thread>

#include "Downloader.h"
#include "MangaSearcher.h"
#include "MangalibAuthorizer.h"
#include "Window/Win32Window.h"
#include <Daedalus.h>

class DownloaderWindow : public Daedalus::Win32Window {
public:
  explicit DownloaderWindow(Daedalus::WindowPropsWin32 props);

  ~DownloaderWindow();

  void Render() override;

private:
  void StartSearch(bool hiRes);

  void DisplayDownload();

  void DisplayLoggingPage();

  void GetTranslations();

  void GetChapters();

  void SelectFolder();

  void DownloadChapters();

  void DisplayDownloadSettings();

  void DisplayDownloadStatus();

  void DisplayTranslationList();

  void DisplaySearch();

  void DrawMangaCard(Manga& manga, bool isClickable = true);

  // Wrapper to get worker state
  template<typename R>
  bool is_ready(std::future<R> const& f)
  {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  }

private:
  const std::string VERSION = "0.2.1";

  char search[512]{};
  char currentName[512]{};
  char login[512]{};
  char password[512]{};

  std::string cookie;
  std::string errorMessage;

  Manga currentManga;

  int width = 100;
  int repeatCount = 3;
  int errorDelay = 0;
  int requestDelay = 0;
  int skipCountStart = 0;
  int skipCountEnd = 0;

  // Worker threads for async
  std::vector<std::future<void>> futures;

  std::vector<Chapter> chapters;
  std::vector<Team> teams;
  std::vector<ImFont*> fonts;

  std::unique_ptr<MangaSearcher> searcher;
  std::unique_ptr<Downloader> downloader;

  enum MenuState
  {
    Login = 0,
    Download,
    Search,
    Logger,
  } state;

  bool isCancelled;
  bool isLogged;// Успешная аутентификация

  bool htmlDownload;
  bool picturesDownload;

  MangalibAuthorizer auth;
};

#endif// DAEDALUS_GUI_H
