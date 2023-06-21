#include "DarkTheme.h"

#include <utility>

DarkTheme::DarkTheme(std::vector<Daedalus::ImGuiFont> primaryFonts,
                     std::vector<Daedalus::ImGuiFont> additionalFonts)
    : ImGuiTheme(std::move(primaryFonts), std::move(additionalFonts))
{}

std::vector<ImFont*> DarkTheme::ApplyTheme(ImGuiIO* io, ImGuiStyle* style)
{
  std::vector<ImFont*> result;
  for(auto& config: fonts) {
    std::vector<ImFont*> tmp = config.ApplyToImGuiIo(io);
    for(const auto& f: tmp) {
      result.emplace_back(f);
    }
  }

  if(!style) {
    return result;
  }

  ImVec4* colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.90f);
  colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.19f, 0.23f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.14f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.09f, 0.10f, 0.13f, 0.71f);
  colors[ImGuiCol_Button] = ImVec4(0.18f, 0.19f, 0.23f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.92f, 0.18f, 0.29f, 0.24f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.70f);
  colors[ImGuiCol_TabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

  // Disable borders (fuck them)
  style->WindowBorderSize = 0.0f;
  style->PopupBorderSize = 0.0f;
  style->FrameBorderSize = 0.0f;

  style->ChildBorderSize = 0.0f;

  // Seems fine
  style->WindowPadding = ImVec2(15, 15);
  style->FramePadding = ImVec2(15.0f, 4.0f);

  style->ItemSpacing = ImVec2(20, 8);
  style->ItemInnerSpacing = ImVec2(10, 6);

  // Big scrollbar = good
  style->ScrollbarSize = 20.0f;
  // Grab big size = good
  style->GrabMinSize = 20.0f;

  // Color button on the left (1 = on the right)
  style->ColorButtonPosition = 0;

  // Title in the center
  style->WindowTitleAlign = ImVec2(0.5f, 0.5f);

  style->WindowRounding = 1.0f;
  style->ScrollbarRounding = 1.0f;
  style->TabRounding = 1.0f;

  style->ChildRounding = 2.0f;
  style->FrameRounding = 3.0f;
  style->PopupRounding = 5.0f;

  style->GrabRounding = 20.0f;
  return result;
}
