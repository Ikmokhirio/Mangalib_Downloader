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
  int errorDelay = 0;
  int requestDealy = 0;

  std::vector<std::thread> threads;
  std::vector<Chapter> chapters;

  std::unique_ptr<Downloader> downloader;

public:
  explicit TestWindow(Daedalus::WindowProps props)
      : Daedalus::Win32Window(std::move(props))
  {
  }

  void Render() override
  {
    ImGui::PushItemWidth(300);
    ImGui::InputText("Link", link, 512);
    ImGui::InputText("Cookie", cookie, 512);

    ImGui::PushItemWidth(128);
    ImGui::DragInt("Width", &width, 1.0f, 1, 100);
    ImGui::DragInt("Requests delay (ms)", &requestDealy, 1.0f, 0, 10000);
    ImGui::DragInt("Pause after an error (s)", &errorDelay, 1.0f, 0, 10000);

    if(ImGui::Button("Get chapters")) {
      auto l = Uri::Parse(link);
      std::vector<Combiner*> combiners = {new HtmlCombiner(width)};
      downloader = std::make_unique<Downloader>(l, cookie, combiners, requestDealy, errorDelay);
      chapters = downloader->GetChapters();
    }

    ImGui::SameLine();

    if(ImGui::Button("Download")) {
      threads.emplace_back([this]() {
        for(auto& ch: chapters) {
          if(ch.selected) {
            downloader->DownloadChapter(ch);
          }
        }
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
      if(ImGui::Selectable(std::format("Volume {0}, chapter {1}", ch.volumeNumber, ch.chapterNumber).c_str(), ch.selected)) {
        ch.selected = !ch.selected;
      }
    }
  }
  ~TestWindow()
  {
    for(auto& t: threads) {
      t.join();
    }
  };
};

Daedalus::Window* CreateGui()
{
  return new TestWindow({"Mangalib downloader", Daedalus::WindowStyle::NoStyle, 420, 240});
}

