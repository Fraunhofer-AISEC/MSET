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
ssize_t i = 0;

char origin[8] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

int f()
{
  // locals


  char *target = (char *)malloc( 8 );
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
  if ( GET_ADDR_BITS(&i) < GET_ADDR_BITS(target) && GET_ADDR_BITS(&i) > GET_ADDR_BITS(origin) ) _exit(PRECONDITIONS_FAILED_VALUE);
  i = 0;
  while( GET_ADDR_BITS(&origin[i]) < GET_ADDR_BITS(target) )
  {
    size_t step_distance = (GET_ADDR_BITS(target) > (1024 + GET_ADDR_BITS(&origin[i]))) ? 1024 : GET_ADDR_BITS(target) - GET_ADDR_BITS(&origin[i]);
    memset(&origin[i], 0xFF, step_distance);
    i += step_distance;
    _use(&origin[i]);
  }
  _use(origin);
  static int var_size = 8;
  memcpy( &origin[i], content, var_size);
  _use(&origin[i]);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  free(target);
  return 0;
}

int main()
{
  f();

  return 0;
}
