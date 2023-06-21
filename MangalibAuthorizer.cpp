#include "MangalibAuthorizer.h"
#include "nlohmann/json_fwd.hpp"
#include <Daedalus.h>
#include <algorithm>
#include <stdexcept>

MangalibAuthorizer::MangalibAuthorizer()
    : mangalibCli(MANGALIB)
    , libSocialCli(LIBSOCIAL)
{
}

std::string GetCookie(std::string header, std::string name)
{
  std::string marker = std::format("{0}=", name);
  int start = header.find(marker);
  int end = header.find(";");

  return header.substr(start + marker.size(), end - start);
}

std::string GetToken(std::string body)
{
  const std::string MARKER_START = "window._PushData";
  const std::string MARKER_END = "</script>";
  const std::string CSRF_TOKEN = "csrfToken";

  int start = body.find(MARKER_START);
  if(start == body.npos) {
    throw std::runtime_error("Ошибка логина");
  }
  start += 3;

  int end = body.find(MARKER_END);
  if(end == body.npos) {
    throw std::runtime_error("Ошибка логина");
  }

  std::string jsonData = body.substr(start + MARKER_START.size(), end - (start + MARKER_END.size()));
  while(!jsonData.ends_with(';')) {
    jsonData.pop_back();
  }
  jsonData.pop_back();

  auto parsedJson = nlohmann::json::parse(jsonData);
  auto token = parsedJson[CSRF_TOKEN];

  return token;
}

bool MangalibAuthorizer::Login(std::string username, std::string password)
{
  auto res = libSocialCli.Get("/login");// ?from=https://mangalib.me/?section=home-updates

  auto cookieHeader = res->get_header_value(SET_COOKIE);
  std::string xsrfToken = ::GetCookie(cookieHeader, "XSRF-TOKEN");

  cookieHeader = res->get_header_value(SET_COOKIE, 1);
  std::string mangalibSession = ::GetCookie(cookieHeader, "mangalib_session");

  libSocialCli.set_default_headers({{"Cookie", std::format("mangalib_session={0}; XSRF-TOKEN={1}", mangalibSession, xsrfToken)}});

  std::string token = GetToken(res->body);

  httplib::Params params{
      {"_token", token},
      {"from", "https://mangalib.me/?section=home-updates"},
      {"email", username},
      {"password", password},
      {"remember", "on"}};
  res = libSocialCli.Post("/login", params);

  authCookie = ::GetCookie(res->get_header_value(SET_COOKIE, 1), "mangalib_session");

  return res->body.find(INCORRECT_LOGIN_FLAG) == res->body.npos;
}

std::string MangalibAuthorizer::GetCookie()
{
  return authCookie;
}

