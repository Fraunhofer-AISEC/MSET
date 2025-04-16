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

struct T s = { {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB} };
struct BigType
{
  char buffer[(size_t)1 << 27];
};

int f()
{
  // locals
  ssize_t i;


  if ( ((ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin)) > 0 && (ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin)) > ((size_t)1 << 27))
       || ((ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin)) < 0 && (ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin))< -((size_t)1 << 27) ) )  _exit(PRECONDITIONS_FAILED_VALUE);
  if ( !((ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin)) >= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  if ( GET_ADDR_BITS(&i) < GET_ADDR_BITS(&((struct BigType *)s.origin)->buffer[(ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin))]) && GET_ADDR_BITS(&i) > GET_ADDR_BITS(&((struct BigType *)s.origin)->buffer[0]) ) _exit(PRECONDITIONS_FAILED_VALUE);
  for (i = 0; i < (ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin)); i++)
  {
    ((struct BigType *)s.origin)->buffer[i] = 0xFF;
  }
  _use(((struct BigType *)s.origin)->buffer);
  for (ssize_t i = 0; i < 8; i++)
  {
    ((struct BigType *)s.origin)->buffer[i + (ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin))] = 0xFF;
  }
  _use(((struct BigType *)s.origin)->buffer);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
