/*
* This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "misc.h"

#include <cassert>
#include <cstring>
#include <dirent.h>
#include <ftw.h>
#include <iostream>
#include <sys/stat.h> // mkdir

static int unlink_content_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
  if ( (typeflag == FTW_D || typeflag == FTW_DP) && ftwbuf->level == 0) return 0; // don't remove the root directory

  if (typeflag == FTW_F || typeflag == FTW_D || typeflag == FTW_DP)
  {
    const int rv = remove(fpath);
    if (rv)
    {
      std::perror("Error removing file from directory");
    }
    return rv;
  }
  return 0;
}

bool directory_exists(const std::string& dir_path)
{
  struct stat info;
  return stat(dir_path.c_str(), &info ) == 0;
}

bool is_directory_empty(const std::string& dir_path)
{
  if ( !directory_exists(dir_path) ) return false;

  DIR* dir = opendir( dir_path.c_str() );
  if (!dir) return false;

  struct dirent* entry;

  bool content_found = false;
  while ( (entry = readdir(dir)) )
  {
    // Skip '.' and '..' entries
    if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0)
    {
      content_found = true;
      break;
    }
  }
  closedir(dir);

  return !content_found;
}

void remove_all_files_from_directory(const std::string& dir_path)
{
  if ( nftw(dir_path.c_str(), unlink_content_cb, 64, FTW_DEPTH) != 0 )
  {
    std::perror(("Error removing directory: " + dir_path).c_str());
    exit(EXIT_FAILURE);
  }
}

void create_directory(const std::string& dir_path)
{
  if ( directory_exists(dir_path) )
  {
    return;
  }
  if ( mkdir(dir_path.c_str(), 0755 ) != 0)
  {
    std::perror("Error creating directory");
    exit(EXIT_FAILURE);
  }
}

