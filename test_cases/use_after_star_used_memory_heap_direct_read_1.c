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

  size_t counter = 0;
  while ( counter < 1000000000)
  {
    free(reallocated);
    reallocated = (char *)malloc( 8 );
    reallocated[0] = 0xAA;
    reallocated[1] = 0xAA;
    reallocated[2] = 0xAA;
    reallocated[3] = 0xAA;
    reallocated[4] = 0xAA;
    reallocated[5] = 0xAA;
    reallocated[6] = 0xAA;
    reallocated[7] = 0xAA;
    if ( GET_ADDR_BITS(target) == GET_ADDR_BITS(reallocated) ) break;
    counter++;}
  if ( counter == 1000000000 ) _exit(PRECONDITIONS_FAILED_VALUE);
  volatile char read_value[8];
  for (ssize_t i = 0; i < 8; i++)
  {
    read_value[i] = target_address[i];
  }
  _use(read_value);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);
  free(reallocated);
  return 0;
}

int main()
{
  f();

  return 0;
}
