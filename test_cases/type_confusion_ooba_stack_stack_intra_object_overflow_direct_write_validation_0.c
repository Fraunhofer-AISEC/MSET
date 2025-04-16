/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Generated by MSET 1.0.
 */

#include <unistd.h> // _exit
#include <stdint.h> // SIZE_MAX
#include <stdlib.h>
#include <string.h>

#ifdef ADDR_MASK
#define GET_ADDR_BITS(p) ((size_t)(p) & ADDR_MASK)
#else
#define GET_ADDR_BITS(p) ((size_t)(p) & (size_t)0xffffffffffffull)
#endif

volatile void *_use(volatile void *p) { return p; }
const char content[8] = "ZZZZZZZ";

// types
struct T
{
  char origin[8];
  char target[8];
};

// globals

struct BigType
{
  char buffer[(size_t)1 << 27];
};

int f()
{
  // locals

  struct T s;

  s.origin[0] = 0xAA;
  s.origin[1] = 0xAA;
  s.origin[2] = 0xAA;
  s.origin[3] = 0xAA;
  s.origin[4] = 0xAA;
  s.origin[5] = 0xAA;
  s.origin[6] = 0xAA;
  s.origin[7] = 0xAA;
  s.target[0] = 0xBB;
  s.target[1] = 0xBB;
  s.target[2] = 0xBB;
  s.target[3] = 0xBB;
  s.target[4] = 0xBB;
  s.target[5] = 0xBB;
  s.target[6] = 0xBB;
  s.target[7] = 0xBB;
  for (ssize_t i = 0; i < 8; i++)
  {
    s.target[i + 0] = 0xFF;
  }
  _use(s.target);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
