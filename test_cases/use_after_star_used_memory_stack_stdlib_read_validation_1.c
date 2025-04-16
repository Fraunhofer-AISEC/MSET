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
int counter = 0;
char *last_address = NULL;
char read_value[8];

int f()
{
  // locals

  char target[8] = "";
  char reallocated[8] = "";

  target[0] = 0xAA;
  target[1] = 0xAA;
  target[2] = 0xAA;
  target[3] = 0xAA;
  target[4] = 0xAA;
  target[5] = 0xAA;
  target[6] = 0xAA;
  target[7] = 0xAA;
  target_address = &target[0];
  last_address = &reallocated[0];
  memcpy( read_value, target_address, 8);
  _use( read_value );

  return 0;
}

int main()
{
  do
  {
    (void)  f();
  } while (counter++ < 100);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}
