/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>


class ArgParser
{
public:
  struct Argument
  {
    bool has_value;
    std::string value_name;     // if has_value
    std::string default_value;  // if has_value
    std::string description;
    bool hidden = false;
  };

  static std::unique_ptr<ArgParser> construct(int argc, const char **argv, const std::vector<std::tuple< std::string, ArgParser::Argument>> &accepted_arguments);

  bool check(const std::string &arg_name) const;
  bool check_and_consume(const std::string &arg_name);
  std::unique_ptr<std::string> get_value_and_consume(const std::string &arg_name);

  bool consumed_everything() const { return args.empty(); }
  std::list<std::string> get_unconsumed() const { return args; }

  ArgParser(const std::list<std::string> &args, const std::map<std::string, Argument> &accepted_args);
private:
  std::list<std::string> args;
  std::map<std::string, Argument> accepted_args;
};
