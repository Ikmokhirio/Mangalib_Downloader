#ifndef MANGALIBDOWNLOADER_DARKTHEME_H
#define MANGALIBDOWNLOADER_DARKTHEME_H

#include <Daedalus.h>

class DarkTheme : public Daedalus::ImGuiTheme{
public:

    DarkTheme(std::vector<Daedalus::ImGuiFont> primaryFonts, std::vector<Daedalus::ImGuiFont> additionalFonts);

    std::vector<ImFont*> ApplyTheme(ImGuiIO *io, ImGuiStyle *style = nullptr) override;

    ~DarkTheme() = default;
};

#endif // MANGALIBDOWNLOADER_DARKTHEME_H
