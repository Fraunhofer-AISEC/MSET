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

struct BigType
{
  char buffer[(size_t)1 << 27];
};
static ssize_t i;

int f()
{
  // locals

  char origin[8] = "";

  origin[0] = 0xAA;
  origin[1] = 0xAA;
  origin[2] = 0xAA;
  origin[3] = 0xAA;
  origin[4] = 0xAA;
  origin[5] = 0xAA;
  origin[6] = 0xAA;
  origin[7] = 0xAA;
  char *target = (char *)malloc( 8 );
  target[0] = 0xAA;
  target[1] = 0xAA;
  target[2] = 0xAA;
  target[3] = 0xAA;
  target[4] = 0xAA;
  target[5] = 0xAA;
  target[6] = 0xAA;
  target[7] = 0xAA;
  if ( ((ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin)) > 0 && (ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin)) > ((size_t)1 << 27))
       || ((ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin)) < 0 && (ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin))< -((size_t)1 << 27) ) )  _exit(PRECONDITIONS_FAILED_VALUE);
  if ( !((ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin)) >= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  if ( GET_ADDR_BITS(&i) < GET_ADDR_BITS(&((struct BigType *)origin)->buffer[(ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin))]) && GET_ADDR_BITS(&i) > GET_ADDR_BITS(&((struct BigType *)origin)->buffer[0]) ) _exit(PRECONDITIONS_FAILED_VALUE);
  for (i = 0; i < (ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin)); i++)
  {
    ((struct BigType *)origin)->buffer[i] = 0xFF;
  }
  _use(((struct BigType *)origin)->buffer);
  for (ssize_t i = 0; i < 8; i++)
  {
    ((struct BigType *)origin)->buffer[i + (ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin))] = 0xFF;
  }
  _use(((struct BigType *)origin)->buffer);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  free(target);
  return 0;
}

int main()
{
  f();

  return 0;
}
