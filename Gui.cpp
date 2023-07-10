#include "Gui.h"
#include "DarkTheme.h"
#include "HtmlCombiner.h"
#include "Images/ImageLoader.h"
#include "Logger.h"
#include "RawCombiner.h"
#include "Themes/ImGuiFont.h"
#include "UriParser.h"
#include "imgui.h"
#include <Daedalus.h>
#include <EntryPoint.h>
#include <Utils/Utils.h>

#include "BinaryFonts/fa_solid.h"
#include "BinaryFonts/source_sans.h"
#include <ImGuiExtension/ImFileBrowser.h>
#include <future>
#include <memory>
#include <thread>

DownloaderWindow::DownloaderWindow(Daedalus::WindowPropsWin32 props)
    : Daedalus::Win32Window(props)
{
  futures.emplace_back(std::async(std::launch::async, []() {}));
  futures.emplace_back(std::async(std::launch::async, []() {}));

  searcher = nullptr;

  state = Login;

  isCancelled = false;

  htmlDownload = false;
  picturesDownload = false;

  std::vector<uint8_t> source_sansFont(source_sans_compressed_size);
  memcpy(source_sansFont.data(), source_sans_compressed_data, source_sans_compressed_size);
  Daedalus::ImGuiFont font(source_sansFont, 14, Daedalus::All);
  Daedalus::ImGuiFont medium(source_sansFont, 18, Daedalus::All);
  Daedalus::ImGuiFont bigFont(source_sansFont, 32, Daedalus::All);
  fonts = SetNextTheme(new DarkTheme({font}, {medium, bigFont}));

  requestDelay = 0;
  errorDelay = 60;
  repeatCount = 5;
}

DownloaderWindow::~DownloaderWindow()
{
  isCancelled = true;
  for(auto& f: futures) {
    f.get();
  }
  if(searcher) {
    for(auto& m: searcher->List()) {
      if(m.img) {
        m.img->Clear();
      }
    }
  }
  Daedalus::ImGuiLogger::DestroyLogger();
}

void DownloaderWindow::DrawMangaCard(Manga& manga, bool isClickable)
{
  if(manga.empty) {
    return;
  }
  const std::string& summary = manga.summary.c_str();
  const float width = 128;      // Ширина картинки
  const float summaryPosX = 160;// Позиция текста с описанием
  const float scrollSizeX = 64;
  const float imagaeOffsetX = 16;
  float rel = 0.0f;
  if(manga.img) {
    rel = (float) manga.img->width / manga.img->height;
  }

  ImGui::PushFont(fonts[1]);
  auto headerSize = ImGui::CalcTextSize(manga.russianName.c_str());
  ImGui::PopFont();

  float textHeight = ImGui::CalcTextSize(summary.c_str(), NULL, false, windowProps.width - summaryPosX - scrollSizeX - imagaeOffsetX).y + headerSize.y * 3;

  float height = width;
  if(manga.img) {
    height = width / rel + headerSize.y * 3;
  } else {
    height = textHeight;
  }

  if(manga.selected) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.92f, 0.18f, 0.29f, 1.00f));
  }
  ImGui::BeginChild(std::to_string(manga.id).c_str(), {(float) windowProps.width, std::max(height, textHeight)}, true);

  ImGui::PushFont(fonts[1]);
  ImGui::SetCursorPosX((windowProps.width - headerSize.x) / 2);
  ImGui::Text("%s", manga.russianName.c_str());
  ImGui::PopFont();

  ImGui::SetCursorPosX(imagaeOffsetX);
  if(manga.img) {
    ImGui::DisplayImage(manga.img, {width, width / rel});
  } else {
    ImGui::Button("EMPTY", {width, width});
  }

  ImGui::SameLine(summaryPosX);
  ImGui::PushTextWrapPos(windowProps.width - scrollSizeX);
  ImGui::Text("%s", summary.c_str());
  ImGui::PopTextWrapPos();

  ImGui::Separator();
  ImGui::EndChild();

  if(manga.selected) {
    ImGui::PopStyleColor();
  }

  if(!isClickable) {
    return;
  }

  manga.selected = ImGui::IsItemHovered();
  if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    currentManga = manga;
    currentManga.selected = false;
    state = Download;
    GetTranslations();
  }
}

void DownloaderWindow::StartSearch(bool hiRes)
{
  if(!is_ready(futures[1])) {
    return;
  }

  for(auto& m: searcher->List()) {
    if(m.img) {
      m.img->Clear();
    }
  }
  futures[1] = std::async(std::launch::async, [this, &hiRes]() {
    try {
      searcher->FindManga(std::string(search), hiRes);
      return;
    } catch(std::exception& e) {
      errorMessage = e.what();
      return;
    }
  });
}

void DownloaderWindow::DisplaySearch()
{
  static bool hiRes;

  ImGui::PushItemWidth(256);
  ImGui::BeginDisabled(!is_ready(futures[1]));
  ImGui::InputTextWithHint("##SEARCH", "Поиск", search, 512);
  ImGui::SameLine();
  ImGui::Checkbox("HiRes обложки", &hiRes);
  if(ImGui::Button("Найти")) {
    errorMessage = "";
    StartSearch(hiRes);
  }
  ImGui::EndDisabled();

  if(!is_ready(futures[1])) {
    return;
  }

  ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());
  for(auto& manga: searcher->List()) {
    if(!manga.img && !manga.thumbnailBinary.empty()) {
      manga.img = LoadImageFromMemory(GetDevicePointer(), manga.thumbnailBinary.data(), manga.thumbnailBinary.size(), 0, 0);
    }
    DrawMangaCard(manga);
  }
}

void DownloaderWindow::Render()
{

  if(state == Login) {
    DisplayLoggingPage();
    return;
  }

  if(ImGui::Button("Поиск")) {
    state = Search;
  }

  ImGui::SameLine();

  if(ImGui::Button("Логи")) {
    state = Logger;
  }

  if(!currentManga.empty) {
    ImGui::SameLine();

    if(ImGui::Button("Загрузка")) {
      state = Download;
    }
  }

  switch(state) {
  case(Download): {
    DisplayDownload();
    break;
  }
  case(Search): {
    DisplaySearch();
    break;
  }
  case(Logger): {
    Daedalus::ImGuiLogger::Draw();
    break;
  }
  }
}

void DownloaderWindow::DisplayLoggingPage()
{
  ImGui::SetCursorPosY(32);
  ImGui::SetCursorPosX(540);
  ImGui::Text("Версия %s", VERSION.c_str());

  ImGui::PushFont(fonts[2]);
  ImGui::SetCursorPosY(windowProps.height / 2.0f - ImGui::CalcTextSize("П").y * 3);

  ImGui::SetCursorPosX((windowProps.width - 384) / 2);
  ImGui::PushItemWidth(384);
  ImGui::InputTextWithHint("##LOGIN", "Логин", login, 512);

  ImGui::SetCursorPosX((windowProps.width - 384) / 2);
  ImGui::PushItemWidth(384);
  ImGui::InputTextWithHint("##PASSWORD", "Пароль", password, 512, ImGuiInputTextFlags_Password);

  ImGui::SetCursorPosX(windowProps.width / 2.0f - 128);
  if(ImGui::Button("Войти", ImVec2{256, 48})) {
    try {
      MangalibAuthorizer auth;
      if(auth.Login(login, password)) {
        cookie = auth.Cookie();
        searcher = std::make_unique<MangaSearcher>(Uri::Parse("https://mangalib.me"), cookie);
        state = Search;
        errorMessage = "";
      } else {
        errorMessage = "Некорректные данные";
      }
    } catch(std::exception& e) {
      errorMessage = e.what();
    }
  }
  ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());
  ImGui::PopFont();
}

void DownloaderWindow::GetTranslations()
{
  chapters = std::vector<Chapter>();

  auto url = Uri::Parse(currentManga.link);

  downloader = std::make_unique<Downloader>(url, cookie, requestDelay, errorDelay, repeatCount);

  try {
    downloader->ExtractJsonData();
    teams = downloader->GetTeams();
  } catch(const MangalibDownloaderError& e) {
    errorMessage = e.what();
  }
}

void DownloaderWindow::GetChapters()
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
}

void DownloaderWindow::SelectFolder()
{
  if(!downloader || chapters.empty()) {
    return;
  }

  if(ImGui::BeginPopup("#FOLDER_SELECT", ImGuiWindowFlags_Modal)) {
    ImGui::Text("Выберите название папки для сохранения : ");

    if(!currentManga.russianName.empty() && ImGui::Button(currentManga.russianName.c_str())) {
      memcpy(currentName, currentManga.russianName.c_str(), 512);
    }
    if(!currentManga.englishName.empty() && ImGui::Button(currentManga.englishName.c_str())) {
      memcpy(currentName, currentManga.englishName.c_str(), 512);
    }
    if(!currentManga.originalName.empty() && ImGui::Button(currentManga.originalName.c_str())) {
      memcpy(currentName, currentManga.originalName.c_str(), 512);
    }

    ImGui::Separator();

    ImGui::Text("Или введите вручную : ");
    ImGui::InputTextWithHint("##FOLDER_NAME", "Папка для сохранения", currentName, 512);
    if(ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      ImGui::Text("Убедитесь, что имя не содержит недопустимых символов");
      ImGui::EndTooltip();
    }
    downloader->SelectName(currentName);

    if(ImGui::Button("Принять")) {
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
      downloader->SetCombiners(combiners);

      DownloadChapters();
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if(ImGui::Button("Отмена")) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void DownloaderWindow::DownloadChapters()
{
  if(!is_ready(futures[0])) {
    return;
  }

  futures[0] = std::async(std::launch::async, [this]() {
    int finished = 0;
    int failed = 0;
    int selected = 0;
    for(auto& ch: chapters) {
      ch.errorOnLastOperation = false;
      ch.finished = false;
      ch.loading = false;
    }
    for(auto& ch: chapters) {
      if(ch.selected) {
        ch.loading = true;
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
        } catch(std::runtime_error& e) {
          DS_ERROR(e.what());
          errorMessage = e.what();
          ch.errorOnLastOperation = true;
          ch.finished = false;
          failed++;
        }
        ch.loading = false;
      }
    }
    DS_INFO("Загрузка завершена. {0}/{1} УСПЕШНО. {2} С ОШИБКОЙ", finished, selected, failed);
  });
}

void DownloaderWindow::DisplayDownloadSettings()
{
  if(currentManga.empty) {
    return;
  }
  DrawMangaCard(currentManga, false);

  if(ImGui::Checkbox("Скачать как html", &htmlDownload)) {
  }

  if(ImGui::Checkbox("Скачать как картинки", &picturesDownload)) {
  }

  ImGui::BeginTable("Settings", 2, 0, ImVec2{(float) windowProps.width, 64});

  ImGui::TableNextRow();

  ImGui::TableNextColumn();
  ImGui::PushItemWidth(48);
  ImGui::DragInt("Ширина страницы [0-100%]", &width, 1.0f, 1, 100);

  ImGui::TableNextColumn();
  ImGui::PushItemWidth(48);
  ImGui::DragInt("Задержка между запросами [мс]", &requestDelay, 1.0f, 0, 10000);

  ImGui::TableNextRow();

  ImGui::TableNextColumn();
  ImGui::PushItemWidth(48);
  ImGui::DragInt("Пауза после ошибки [с]", &errorDelay, 1.0f, 0, 10000);

  ImGui::TableNextColumn();
  ImGui::PushItemWidth(48);
  ImGui::DragInt("Количество попыток на картинку", &repeatCount, 1.0f, 1, 100);

  ImGui::EndTable();
}

void DownloaderWindow::DisplayDownloadStatus()
{
  for(auto& ch: chapters) {
    std::string selectionText;
    if(ch.finished) {
      selectionText = std::format("Том {0}, глава {1} {2}", ch.volumeNumber, ch.chapterNumber, "[Успешно]");
    } else if(ch.errorOnLastOperation) {
      selectionText = std::format("Том {0}, глава {1} {2}", ch.volumeNumber, ch.chapterNumber, "[Ошибка]");
    } else if(ch.loading) {
      selectionText = std::format("Том {0}, глава {1} {2}", ch.volumeNumber, ch.chapterNumber, "[Загрузка...]");
    } else {
      selectionText = std::format("Том {0}, глава {1}", ch.volumeNumber, ch.chapterNumber);
    }

    if(ImGui::Selectable(selectionText.c_str(), ch.selected)) {
      ch.selected = !ch.selected;
    }
  }
}

void DownloaderWindow::DisplayTranslationList()
{
  if(!downloader) {
    return;
  }

  if(teams.empty()) {
    ImGui::Text("Произведение не содержит альтернативных переводов");
    if(ImGui::Button("Выгрузить главы")) {
      downloader->SetErrorDelay(errorDelay);
      downloader->SetMaxAttempts(repeatCount);
      downloader->SetRequestDelay(requestDelay);
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

void DownloaderWindow::DisplayDownload()
{
  DisplayDownloadSettings();

  ImGui::Text("Как выбрать настройки?");
  ImGui::Text(R"(Основная проблема при скачивании - сервер перестает отвечать
Независимо от ошибки эта проблема решается с помощью паузы между запросами
Есть два варианта:
1) Скачиваешь главы, которые скачиваются и игнорируешь те, 
   которые не скачались, а потом докачиваешь остальные
2) Скачиваешь главы по порядку и ждёшь, пока сервер начнет отвечать

В первом случае достаточно поставить 3 попытки и "паузу после ошибки" - 1 или 2 секунды

Во втором случае я предлагаю поставить 5 попыток и "паузу после ошибки" - 60 секунд, то есть минуту. 
Тогда время скачивания будет долгим, но больше вероятность, что все главы скачаются за один запуск
Для второго случая предназначены настройки по-умолчанию)");

  ImGui::Separator();

  DisplayTranslationList();

  SelectFolder();

  if(!chapters.empty()) {

    ImGui::Text("Список глав (выберите нужные и нажмите скачать)");
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
      ImGui::OpenPopup("#FOLDER_SELECT");
    }
  }

  ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());

  DisplayDownloadStatus();
}

#include "Windows/Resource.h"

Daedalus::Window* CreateGui()
{
  // Daedalus::Logger::InitLogger();
  Daedalus::ImGuiLogger::InitLogger();

  Daedalus::WindowPropsWin32 props{"Мангалиб загрузчик", Daedalus::WindowStyle::NoStyle, 640, 480};
  props.bigIcon = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 256, 256, 0);
  props.smallIcon = (HICON) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 96, 96, 0);

  return new DownloaderWindow(props);
}

