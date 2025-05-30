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


int f()
{
  // locals

  char origin[8] = "";
  char target[8] = "";

  origin[0] = 0xAA;
  origin[1] = 0xAA;
  origin[2] = 0xAA;
  origin[3] = 0xAA;
  origin[4] = 0xAA;
  origin[5] = 0xAA;
  origin[6] = 0xAA;
  origin[7] = 0xAA;
  target[0] = 0xAA;
  target[1] = 0xAA;
  target[2] = 0xAA;
  target[3] = 0xAA;
  target[4] = 0xAA;
  target[5] = 0xAA;
  target[6] = 0xAA;
  target[7] = 0xAA;
  _use(target);
  _use(origin);
  if ( !(-(ssize_t)(GET_ADDR_BITS(origin) - GET_ADDR_BITS(target)) >= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  ssize_t i = 0;
  char tmp[1024];
  while( i < -(ssize_t)(GET_ADDR_BITS(origin) - GET_ADDR_BITS(target)) )
  {
    size_t step_distance = (-(ssize_t)(GET_ADDR_BITS(origin) - GET_ADDR_BITS(target)) > (1024 + i)) ? 1024 : -(ssize_t)(GET_ADDR_BITS(origin) - GET_ADDR_BITS(target)) - i;
    memcpy((void *)tmp, &origin[i], step_distance);
    i += step_distance;
    _use(&tmp);
  }
  char read_value[8];
  memcpy( read_value, &origin[i], 8);
  _use( read_value );
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
