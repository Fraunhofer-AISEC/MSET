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
  char target[8];
  char origin[8];
};

// globals

ssize_t i;

int f()
{
  // locals

  struct T s;

  s.target[0] = 0xAA;
  s.target[1] = 0xAA;
  s.target[2] = 0xAA;
  s.target[3] = 0xAA;
  s.target[4] = 0xAA;
  s.target[5] = 0xAA;
  s.target[6] = 0xAA;
  s.target[7] = 0xAA;
  s.origin[0] = 0xBB;
  s.origin[1] = 0xBB;
  s.origin[2] = 0xBB;
  s.origin[3] = 0xBB;
  s.origin[4] = 0xBB;
  s.origin[5] = 0xBB;
  s.origin[6] = 0xBB;
  s.origin[7] = 0xBB;
  _use(s.target);
  _use(s.origin);
  if ( !(-(ssize_t)(GET_ADDR_BITS(s.origin) - GET_ADDR_BITS(s.target)) >= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  if ( GET_ADDR_BITS(&i) < GET_ADDR_BITS(s.target) && GET_ADDR_BITS(&i) > GET_ADDR_BITS(s.origin) ) _exit(PRECONDITIONS_FAILED_VALUE);
  i = 0;
  while( GET_ADDR_BITS(&s.origin[i]) < GET_ADDR_BITS(s.target) )
  {
    size_t step_distance = (GET_ADDR_BITS(s.target) > (1024 + GET_ADDR_BITS(&s.origin[i]))) ? 1024 : GET_ADDR_BITS(s.target) - GET_ADDR_BITS(&s.origin[i]);
    memset(&s.origin[i], 0xFF, step_distance);
    i += step_distance;
    _use(&s.origin[i]);
  }
  _use(s.origin);
  static int var_size = 8;
  memcpy( &s.origin[i], content, var_size);
  _use(&s.origin[i]);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
