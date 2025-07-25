/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Generated by MSET 1.1.
 */

/*
 * Origin: global
 * Target: heap
 * Bug type: inter-object, linear OOBA, overflow
 * Access type: direct, write
 * Variant: target declared after origin, target reached by writing using an index, stack auxiliary variables, declared last
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

char origin[8] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

int f()
{
  // locals

  ssize_t reach_index = 0;

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
  while( GET_ADDR_BITS(&target[reach_index]) != GET_ADDR_BITS(target) )
  {
    target[reach_index] = 0xFF;
    ++reach_index;
    _use(&target[reach_index]);
  }
  volatile size_t i;
  for (i = 0; i < 8; i++)
  {
    (target + reach_index)[i] = 0xFF;
  }
  _use((target + reach_index));
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  free(target);
  return 0;
}

int main()
{
  f();

  return 0;
}
