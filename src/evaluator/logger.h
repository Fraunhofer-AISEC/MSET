/*
* This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once


class Logger
{
public:
  enum class _log_level_t
  {
    NOT_ALLOWED = 0,
    NORMAL = 1,
    VERBOSE = 2,
  };
  static _log_level_t allowed_log_level;

  explicit Logger(_log_level_t log_level):
    log_level(log_level)
  {
  }

  template<class T>
  Logger &operator<<(const T &message)
  {
    if (log_level <= allowed_log_level)
    {
      std::cout << message; // always log
      std::cout.flush();
    }
    return *this;
  }

private:
  _log_level_t log_level;
};

using log_level_t = Logger::_log_level_t;
