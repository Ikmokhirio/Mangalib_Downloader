#include "Downloader.h"
#include "HtmlCombiner.h"
#include "MangalibAuthorizer.h"
#include "UriParser.h"
#include "imgui.h"
#include <Daedalus.h>
#include <EntryPoint.h>
#include <memory>
#include <thread>

class TestWindow : public Daedalus::Win32Window {
private:
  char link[512]{};
  std::string cookie;
  std::string errorMessage;
  char login[512]{};
  char password[512]{};

  int width = 100;
  int repeatCount = 3;
  int errorDelay = 0;
  int requestDealy = 0;

  std::vector<std::thread> threads;
  std::vector<Chapter> chapters;

  std::unique_ptr<Downloader> downloader;

  bool isCancelled;
  bool isLogged;
  bool isWorkFinished;

  MangalibAuthorizer auth;

  void DisplayLoggingPage()
  {
    ImGui::InputText("Email", login, 512);
    ImGui::InputText("Password", password, 512, ImGuiInputTextFlags_Password);
    if(ImGui::Button("Login")) {
      MangalibAuthorizer auth;
      if(auth.Login(login, password)) {
        cookie = auth.GetCookie();
        isLogged = true;
        errorMessage = "";
      } else {
        errorMessage = "Incorrect auth data";
      }
    }

    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());
  }

  void GetChapters()
  {
    auto url = Uri::Parse(link);
    std::vector<Combiner*> combiners = {new HtmlCombiner(width)};
    downloader = std::make_unique<Downloader>(url, cookie, combiners, requestDealy, errorDelay, repeatCount);
    try {
      chapters = downloader->GetChapters();
      errorMessage = "";
    } catch(MangalibDownloaderError& e) {
      errorMessage = "Cannot get chapters. ";
      errorMessage.append(e.what());
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
        DS_INFO("Finished downloading. {0}/{1} SUCCESS. {2}/{1} FAILED", finished, selected, failed);
        isWorkFinished = true;
      });
    }
  }

  void DisplaySettings()
  {
    ImGui::PushItemWidth(300);
    ImGui::InputText("Link", link, 512);

    ImGui::BeginTable("Settings", 2);

    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Width [0-100%]", &width, 1.0f, 1, 100);

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Requests delay [ms]", &requestDealy, 1.0f, 0, 10000);

    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Error pause [s]", &errorDelay, 1.0f, 0, 10000);

    ImGui::TableNextColumn();
    ImGui::PushItemWidth(48);
    ImGui::DragInt("Repeat count", &repeatCount, 1.0f, 1, 100);

    ImGui::EndTable();

    if(ImGui::Button("Get chapters")) {
      GetChapters();
    }

    ImGui::SameLine();

    if(ImGui::Button("Download")) {
      DownloadChapters();
    }
  }

  void DisplaySelection()
  {
    if(chapters.size()) {
      if(ImGui::Button("Select all")) {
        for(auto& ch: chapters) {
          ch.selected = true;
        }
      }

      ImGui::SameLine();

      if(ImGui::Button("Deselect all")) {
        for(auto& ch: chapters) {
          ch.selected = false;
        }
      }
    }
  }

  void DisplayDownloadStatus()
  {
    for(auto& ch: chapters) {
      std::string selectionText;
      if(ch.finished) {
        selectionText = std::format("Volume {0}, chapter {1} {2}", ch.volumeNumber, ch.chapterNumber, "[V]");
      } else if(ch.errorOnLastOperation) {
        selectionText = std::format("Volume {0}, chapter {1} {2}", ch.volumeNumber, ch.chapterNumber, "[X]");
      } else {
        selectionText = std::format("Volume {0}, chapter {1}", ch.volumeNumber, ch.chapterNumber);
      }

      if(ImGui::Selectable(selectionText.c_str(), ch.selected)) {
        ch.selected = !ch.selected;
      }
    }
  }

public:
  explicit TestWindow(Daedalus::WindowProps props)
      : Daedalus::Win32Window(std::move(props))
  {
    isCancelled = false;
    isLogged = false;
    isWorkFinished = true;
  }

  void Render() override
  {
    if(!isLogged) {
      DisplayLoggingPage();
      return;
    }

    DisplaySettings();

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
  };
};

Daedalus::Window* CreateGui()
{
  return new TestWindow({"Mangalib downloader", Daedalus::WindowStyle::NoStyle, 420, 240});
}

