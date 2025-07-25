/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <memory>
#include <stdexcept>
#include <string>

inline bool is_number(const std::string& s)
{
  try
  {
    (void)std::stoll(s);
    return true;
  }
  catch (const std::invalid_argument& e)
  {
    return false;
  }
  catch (const std::out_of_range& e)
  {
    return false;
  }
}

template <typename T, typename U>
bool is_a(const std::shared_ptr<U>& ptr)
{
  return std::dynamic_pointer_cast<T>(ptr) != nullptr;
}

template <typename T, typename U>
bool are_the_same_type(const std::shared_ptr<T>& a, const std::shared_ptr<U>& b)
{
  return a && b && typeid(*a) == typeid(*b);
}

extern bool directory_exists(const std::string& dir_path);
extern bool is_directory_empty(const std::string& dir_path);
extern void remove_all_files_from_directory(const std::string& dir_path);
extern void create_directory(const std::string& dir_path);
