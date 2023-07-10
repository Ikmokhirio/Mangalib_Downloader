#ifndef MANGALIB_DOWNLOADER_MANGALIBAUTHORIZER_H
#define MANGALIB_DOWNLOADER_MANGALIBAUTHORIZER_H

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "Error.h"
#include <httplib.h>
#include <iostream>
#include <nlohmann/json.hpp>

class MangalibAuthorizer {
private:
  const std::string LIBSOCIAL = "https://lib.social";
  const std::string MANGALIB = "https://mangalib.me";
  const std::string CSRF_TOKEN = "csrfToken";
  const std::string SET_COOKIE = "set-cookie";
  const std::string INCORRECT_LOGIN_FLAG = "Redirecting to http://lib.social/login";

  httplib::Client mangalibCli;
  httplib::Client libSocialCli;

  std::string authCookie;

public:
  MangalibAuthorizer();

  bool Login(std::string username, std::string password);

  std::string Cookie();
};

#endif// MANGALIB_DOWNLOADER_MANGALIBAUTHORIZER_H
