/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Generated by MSET 1.1.
 */

/*
 * Origin: global
 * Target: global
 * Bug type: inter-object, linear OOBA, overflow
 * Access type: stdlib, write
 * Variant: target declared before origin, distance is negated before checking, target reached by stdlib writing using an index, target accessed by stdlib writing using constants, global auxiliary variables, declared last
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
char origin[8] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
volatile ssize_t i = 0;
volatile size_t step_distance;

int f()
{
  // locals


  _use(target);
  _use(origin);
  if ( !(-(ssize_t)(GET_ADDR_BITS(origin) - GET_ADDR_BITS(target)) >= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  if ( GET_ADDR_BITS(&i) < GET_ADDR_BITS(target) && GET_ADDR_BITS(&i) > GET_ADDR_BITS(origin) ) _exit(PRECONDITIONS_FAILED_VALUE);
  if ( GET_ADDR_BITS(&step_distance) < GET_ADDR_BITS(target) && GET_ADDR_BITS(&step_distance) > GET_ADDR_BITS(origin) ) _exit(PRECONDITIONS_FAILED_VALUE);
  i = 0;
  while( GET_ADDR_BITS(&origin[i]) < GET_ADDR_BITS(target) )
  {
    step_distance = (GET_ADDR_BITS(target) > (1024 + GET_ADDR_BITS(&origin[i]))) ? 1024 : GET_ADDR_BITS(target) - GET_ADDR_BITS(&origin[i]);
    memset((void *)&origin[i], 0xFF, step_distance);
    i += step_distance;
    _use(&origin[i]);
  }
  _use(origin);
  memset( (void *)&origin[i], 0xFF, 8);
  _use(&origin[i]);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
