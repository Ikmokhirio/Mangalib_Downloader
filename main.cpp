#include "Downloader.h"
#include "HtmlCombiner.h"
#include "UriParser.h"
#include "imgui.h"
#include <Daedalus.h>
#include <EntryPoint.h>
#include <thread>

class TestWindow : public Daedalus::Win32Window
{
private:
  char link[512]{};
  char cookie[512]{};
  int width = 100;
  int errorDelay = 0;
  int requestDealy = 0;

  std::vector<std::thread> threads;

public:
  explicit TestWindow(Daedalus::WindowProps props)
      : Daedalus::Win32Window(std::move(props))
  {
  }

  void Render() override
  {
    ImGui::InputText("Link", link, 512);
    ImGui::InputText("Cookie", cookie, 512);
    ImGui::DragInt("Width", &width, 1.0f, 1, 100);
    ImGui::DragInt("Requests delay (ms)", &requestDealy, 1.0f, 0, 10000);
    ImGui::DragInt("Pause after an error (s)", &errorDelay, 1.0f, 0, 10000);

    if(ImGui::Button("Download")) {
      threads.emplace_back([this]() {
        auto l = link;
        auto c = cookie;
        auto w = width;
        Downloader downloader(Uri::Parse(l), c, {new HtmlCombiner(w)}, requestDealy, errorDelay);
        downloader.Download();
      });
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
  return new TestWindow({"Example window", Daedalus::WindowStyle::NoStyle, 1280, 800});
}

//int main()
//{
//  Logger::InitLogger();

//  const std::string url = "https://mangalib.me/geomsulmyeong-ga-magnaeadeul?section=chapters&ui=382881";
//  Downloader downloader(Uri::Parse(url), "eyJpdiI6Ii9JZXNlcnlUa0VmNlRSZ1lXNnRPL0E9PSIsInZhbHVlIjoiSUhOSUEzbHM2V0ZOVFdpYjUyVDhwazZKbm1iWTlNbEZ0bDJYNHl4MWpwbUhRYnB4OUhrQ1FiQlNtc1R0TFVmYnVCa0tmSUNNbTA4S3JKU2M3eUZnejVaNzhFZ1pzWG5oNW9jbnd0Y2h2SEIwcE0vbHpKUFUvRmZMOGpEdHY4ZnYiLCJtYWMiOiI4NTJhNmNjZmY1N2ZjMWRiZjViY2E5ZjUzNTNmMTg5MWU3YzFjNDI1M2JlN2UwYTIyMGExMmE3YzYzMDEzNDg0IiwidGFnIjoiIn0=",
//                        {new HtmlCombiner});
//
// downloader.Download();
//}
