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

// globals

ssize_t i;

int f()
{
  // locals

  char target[8] = "";
  char origin[8] = "";

  target[0] = 0xAA;
  target[1] = 0xAA;
  target[2] = 0xAA;
  target[3] = 0xAA;
  target[4] = 0xAA;
  target[5] = 0xAA;
  target[6] = 0xAA;
  target[7] = 0xAA;
  origin[0] = 0xAA;
  origin[1] = 0xAA;
  origin[2] = 0xAA;
  origin[3] = 0xAA;
  origin[4] = 0xAA;
  origin[5] = 0xAA;
  origin[6] = 0xAA;
  origin[7] = 0xAA;
  _use(target);
  _use(origin);
  i = 0;
  memcpy(target, content, 0);
  _use(target);
  i = 0;
  static int var_size = 8;
  memcpy( &target[i], content, var_size);
  _use(&target[i]);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
