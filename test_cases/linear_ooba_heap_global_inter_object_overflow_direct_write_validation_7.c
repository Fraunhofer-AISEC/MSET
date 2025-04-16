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

char target[8] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
ssize_t reach_index = 0;

int f()
{
  // locals


  char *origin = (char *)malloc( 8 );
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
  reach_index = 0;
  while( GET_ADDR_BITS(&target[reach_index]) != GET_ADDR_BITS(target) )
  {
    target[reach_index] = 0xFF;
    ++reach_index;
    _use(&target[reach_index]);
  }
  
  for (ssize_t i = 0; i < 8; i++)
  {
    (target + reach_index)[i] = 0xFF;
  }
  _use((target + reach_index));
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  free(origin);
  return 0;
}

int main()
{
  f();

  return 0;
}
