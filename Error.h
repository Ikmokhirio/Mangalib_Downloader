#ifndef LOKI_MANGALIBERROR_H
#define LOKI_MANGALIBERROR_H

#include "Logger.h"
#include <stdexcept>

class MangalibDownloaderError : public std::runtime_error {
public:
  inline MangalibDownloaderError(std::string errorMessage)
      : std::runtime_error(errorMessage)
  {
    DS_ERROR(errorMessage.c_str());
  }
};

#endif// LOKI_MANGALIBERROR_H

