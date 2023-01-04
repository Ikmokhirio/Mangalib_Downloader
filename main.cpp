#include "Downloader.h"
#include "HtmlCombiner.h"
#include "UriParser.h"
#include "imgui.h"
#include <Daedalus.h>
#include <EntryPoint.h>
#include <memory>
#include <thread>

class TestWindow : public Daedalus::Win32Window {
private:
  char link[512]{};
  char cookie[512]{};

  int width = 100;
  int repeatCount = 3;
  int errorDelay = 0;
  int requestDealy = 0;

  std::vector<std::thread> threads;
  std::vector<Chapter> chapters;

  std::unique_ptr<Downloader> downloader;

  bool isCancelled;

public:
  explicit TestWindow(Daedalus::WindowProps props)
      : Daedalus::Win32Window(std::move(props))
  {
    isCancelled = false;
  }

  void Render() override
  {
    ImGui::PushItemWidth(300);
    ImGui::InputText("Link", link, 512);
    ImGui::InputText("Cookie", cookie, 512);

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
      auto l = Uri::Parse(link);
      std::vector<Combiner*> combiners = {new HtmlCombiner(width)};
      downloader = std::make_unique<Downloader>(l, cookie, combiners, requestDealy, errorDelay, repeatCount);
      chapters = downloader->GetChapters();
    }

    ImGui::SameLine();

    if(ImGui::Button("Download")) {
      threads.emplace_back([this]() {
        int finished = 0;
        int failed = 0;
        for(auto& ch: chapters) {
          if(ch.selected) {
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
        DS_INFO("Finished downloading. {0}/{1} SUCCESS. {2}/{1} FAILED", finished, chapters.size(), failed);
      });
    }

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

