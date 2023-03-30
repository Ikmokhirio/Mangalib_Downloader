#include "DarkTheme.h"
#include "Downloader.h"
#include "HtmlCombiner.h"
#include "Logger.h"
#include "MangalibAuthorizer.h"
#include "RawCombiner.h"
#include "Themes/ImGuiFont.h"
#include "UriParser.h"
#include "imgui.h"
#include <Daedalus.h>
#include <EntryPoint.h>
#include <Utils/Utils.h>
#include <memory>
#include <sstream>
#include <thread>

class TestWindow : public Daedalus::Win32Window {
private:
  char link[512]{};
  std::string cookie;
  std::string errorMessage;
  char login[512]{};
  char password[512]{};

  std::wstring rusName;
  std::wstring origName;
  std::wstring engName;
  std::string currentName;

  int width = 100;
  int repeatCount = 3;
  int errorDelay = 0;
  int requestDealy = 0;

  std::vector<std::thread> threads;
  std::vector<Chapter> chapters;
  std::vector<Team> teams;
  std::vector<ImFont*> fonts;

  std::unique_ptr<Downloader> downloader;

  bool drawLogger;
  bool isCancelled;
  bool isLogged;
  bool isWorkFinished;

  bool htmlDownload;
  bool picturesDownload;

  MangalibAuthorizer auth;

  void DisplayLoggingPage()
  {
    ImGui::PushFont(fonts[1]);
    ImGui::SetCursorPosY(windowProps.height / 2.0f - ImGui::CalcTextSize("П").y * 3);
    ImGui::Text("Логин : ");
    ImGui::SameLine();
    ImGui::SetCursorPosX(windowProps.width - 496);
    ImGui::InputText("##LOGIN", login, 512);

    ImGui::Text("Пароль : ");
    ImGui::SameLine();
    ImGui::SetCursorPosX(windowProps.width - 496);
    ImGui::InputText("##PASSWORD", password, 512, ImGuiInputTextFlags_Password);

    ImGui::SetCursorPosX(windowProps.width / 2.0f - 128);
    if(ImGui::Button("Войти", ImVec2{256, 48})) {
      MangalibAuthorizer auth;
      if(auth.Login(login, password)) {
        cookie = auth.GetCookie();
        isLogged = true;
        errorMessage = "";
      } else {
        errorMessage = "Некорректные данные";
      }
    }
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());
    ImGui::PopFont();
  }

  void GetTranslations()
  {
    auto url = Uri::Parse(link);
    std::vector<Combiner*> combiners;

    if(htmlDownload) {
      combiners.emplace_back(new HtmlCombiner(width));
    }
    if(picturesDownload) {
      combiners.emplace_back(new RawCombiner());
    }
    if(combiners.empty()) {
      errorMessage = "Выберите один или несколько вариантов для загрузки";
      return;
    } else {
      errorMessage = "";
    }

    downloader = std::make_unique<Downloader>(url, cookie, combiners, requestDealy, errorDelay, repeatCount);

    try {
      downloader->ExtractJsonData();
      teams = downloader->GetTeams();
    } catch(const MangalibDownloaderError& e) {
      errorMessage = e.what();
    }
  }

  void GetChapters()
  {
    if(!downloader) {
      return;
    }

    try {
      chapters = downloader->GetChapters();
      errorMessage = "";
    } catch(MangalibDownloaderError& e) {
      errorMessage = "Главы не загружены. ";
      errorMessage.append(e.what());
    }

    rusName = downloader->GetRussianName();
    engName = downloader->GetEnglishName();
    origName = downloader->GetOriginalName();
  }

  void SelectName()
  {
    if(!downloader || chapters.empty()) {
      return;
    }

    ImGui::Text("Выберите название папки для сохранения : ");
    if(!currentName.empty()) {
      ImGui::Text("Текущее имя : %s", currentName.c_str());
    }

    if(!rusName.empty() && ImGui::Button(Converter::ToString(rusName).c_str())) {
      downloader->SelectName(rusName);
      currentName = Converter::ToString(rusName);
    }
    if(!engName.empty() && ImGui::Button(Converter::ToString(engName).c_str())) {
      downloader->SelectName(engName);
      currentName = Converter::ToString(engName);
    }
    if(!origName.empty() && ImGui::Button(Converter::ToString(origName).c_str())) {
      downloader->SelectName(origName);
      currentName = Converter::ToString(origName);
    }
  }

  void DownloadChapters()
  {
    if(isWorkFinished) {
      isWorkFinished = false;
      threads.emplace_back([this]() { 
        int finished = 0;
        int failed = 0;
        int selected = 0;
        for(auto& ch: chapters) {
          if(ch.selected) {
            selected++;
            if(isCancelled) {
              return;
            }
            try {
              downloader->DownloadChapter(ch);
              ch.errorOnLastOperation = false;
              ch.finished = true;
              finished++;
            } catch(MangalibDownloaderError& e) {
              ch.errorOnLastOperation = true;
              ch.finished = false;
              failed++;
            }
          }
        }
        DS_INFO("Загрузка завершена. {0}/{1} УСПЕШНО. {2} С ОШИБКОЙ", finished, selected, failed);
        isWorkFinished = true;
      });
    }
  }

  void DisplaySettings()
  {
    ImGui::PushItemWidth(300);
    ImGui::InputText("Ссылка", link, 512);

    if(ImGui::Selectable("Скачать как html", htmlDownload)) {
      htmlDownload = !htmlDownload;
    }

    if(ImGui::Selectable("Скачать как картинки", picturesDownload)) {
      picturesDownload = !picturesDownload;
    }

    if(ImGui::Button("Получить список переводов")) {
      GetTranslations();
    }

    ImGui::BeginTable("Settings", 2, 0, ImVec2{(float) windowProps.width, 64});

    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Ширина страницы [0-100%]", &width, 1.0f, 1, 100);

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Задержка между запросами [мс]", &requestDealy, 1.0f, 0, 10000);

    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Пауза после ошибки [с]", &errorDelay, 1.0f, 0, 10000);

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Количество повторений после ошибок", &repeatCount, 1.0f, 1, 100);

    ImGui::EndTable();

    SelectName();

    ImGui::Separator();
  }

  void DisplaySelection()
  {
    if(chapters.empty()) { return; }

    if(ImGui::Button("Выбрать все")) {
      for(auto& ch: chapters) {
        ch.selected = true;
      }
    }

    ImGui::SameLine();

    if(ImGui::Button("Убрать все")) {
      for(auto& ch: chapters) {
        ch.selected = false;
      }
    }

    ImGui::SameLine();

    if(ImGui::Button("Скачать")) {
      DownloadChapters();
    }
  }

  void DisplayDownloadStatus()
  {

    for(auto& ch: chapters) {
      std::string selectionText;
      if(ch.finished) {
        selectionText = std::format("Том {0}, глава {1} {2}", ch.volumeNumber, ch.chapterNumber, "[Успешно]");
      } else if(ch.errorOnLastOperation) {
        selectionText = std::format("Том {0}, глава {1} {2}", ch.volumeNumber, ch.chapterNumber, "[Ошибка]");
      } else {
        selectionText = std::format("Том {0}, глава {1}", ch.volumeNumber, ch.chapterNumber);
      }

      if(ImGui::Selectable(selectionText.c_str(), ch.selected)) {
        ch.selected = !ch.selected;
      }
    }
  }

  void DisplayTranslations()
  {
    if(!downloader) {
      return;
    }

    if(teams.empty()) {
      ImGui::Text("Произведение не содержит альтернативных переводо");
      if(ImGui::Button("Выгрузить главы")) {
        GetChapters();
      }
      return;
    }

    ImGui::Text("Выберите перевод");
    for(auto& team: teams) {
      if(ImGui::Button(team.name.c_str())) {
        downloader->SelectBranch(team.branch);
        GetChapters();
      }
    }
  }

public:
  explicit TestWindow(Daedalus::WindowProps props)
      : Daedalus::Win32Window(std::move(props))
  {

    drawLogger = false;
    isCancelled = false;
    isLogged = false;
    isWorkFinished = true;

    htmlDownload = false;
    picturesDownload = false;

    Daedalus::ImGuiFont font(R"(Fonts\Inter-Bold.ttf)", 14, Daedalus::Russian | Daedalus::English);
    Daedalus::ImGuiFont bigFont(R"(Fonts\Inter-Bold.ttf)", 32, Daedalus::Russian | Daedalus::English);
    fonts = SetNextTheme(new DarkTheme({font}, {bigFont}));
  }

  void Render() override
  {
    if(!isLogged) {
      DisplayLoggingPage();
      return;
    }

    if(ImGui::Button("Включить логи")) {
      drawLogger = true;
    }

    ImGui::SameLine();

    if(ImGui::Button("Выключить логи")) {
      drawLogger = false;
    }

    if(drawLogger) {
      Daedalus::ImGuiLogger::Draw();
      return;
    }

    DisplayTranslations();

    DisplaySettings();

    ImGui::Text("Список глав");

    DisplaySelection();

    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());

    DisplayDownloadStatus();
  }

  ~TestWindow()
  {
    isCancelled = true;
    for(auto& t: threads) {
      t.join();
    }
    Daedalus::ImGuiLogger::DestroyLogger();
  };
};

Daedalus::Window* CreateGui()
{
  return new TestWindow({"Мангалиб загрузчик", Daedalus::WindowStyle::NoStyle, 640, 480});
}

