/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "arg_parser.h"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <set>

bool starts_with(const std::string& str, const std::string& prefix)
{
  return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

ArgParser::ArgParser(const std::list<std::string> &args, const std::map<std::string, Argument> &accepted_args):
  args(args),
  accepted_args(accepted_args)
{
}

std::unique_ptr<ArgParser> ArgParser::construct(int argc, const char **argv,const std::vector<std::tuple< std::string, ArgParser::Argument>> &accepted_args)
{
  std::set<std::string> args_set;
  std::list<std::string> args_list;
  std::map<std::string, Argument> accepted_args_map;

  for (auto accepted_arg: accepted_args)
  {
    accepted_args_map[std::get<0>(accepted_arg)] = std::get<1>(accepted_arg);
  }

  bool expect_value = false;
  for (int i = 1; i < argc; i++)
  {
    if (!expect_value)
    {
      const auto res = args_set.insert(argv[i]);
      if (!res.second)
      {
        std::cerr << "WARNING: duplicate argument: " << argv[i] << std::endl;
      }
      else
      {
        auto it = accepted_args_map.find(argv[i]);
        if (it == accepted_args_map.end())
        {
          std::cerr << "ERROR: argument " << argv[i] << " not accepted." << std::endl;
          return nullptr;
        }
        expect_value = it->second.has_value;
      }
    }
    else
    {
      expect_value = false;
    }
    args_list.emplace_back(argv[i]);
  }

  return std::make_unique<ArgParser>(args_list, accepted_args_map);
}


bool ArgParser::check_and_consume(const std::string &arg_name)
{
  auto it = std::find(args.begin(), args.end(), arg_name);
  if (it == args.end())
  {
    return false;
  }
  args.erase(it);
  return true;
}


bool ArgParser::check(const std::string &arg_name) const
{
  auto it = std::find(args.begin(), args.end(), arg_name);
  if (it == args.end())
  {
    return false;
  }
  return true;
}

std::unique_ptr<std::string> ArgParser::get_value_and_consume(const std::string &arg_name)
{
  const auto it = std::find(args.begin(), args.end(), arg_name);
  if ( it != args.end() )
  {
    if ( std::next(it) != args.end() )
    {
      std::string value = *std::next(it);
      if (starts_with(value, "--") || starts_with(value, "-"))
      {
        std::cerr << "ERROR: invalid value for \"" << arg_name << "\": \"" << value << "\"" << std::endl;
        return nullptr;
      }

      args.erase(std::next(it));
      args.erase(it);
      return std::make_unique<std::string>(value);
    }
    std::cerr << "ERROR: missing argument value for: " << arg_name << std::endl;
    return nullptr;
  }

  return nullptr;
}

