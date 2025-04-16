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

char *target_address;

int f()
{
  // locals


  char *target = (char *)malloc( 8 );
  
  target_address = &target[0];

  free(target);
  char *reallocated = (char *)malloc( 8 );

  if ( GET_ADDR_BITS(target) != GET_ADDR_BITS(reallocated) ) _exit(PRECONDITIONS_FAILED_VALUE);
  static int var_size = 8;
  memcpy( target_address, content, var_size);
  _use(target_address);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);
  free(reallocated);
  return 0;
}

int main()
{
  f();

  return 0;
}
