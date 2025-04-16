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

int f()
{
  // locals


  _use(s.target);
  _use(s.origin);
  if ( !((ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin)) <= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  volatile char tmp;
  char *aux_ptr = &s.origin[0];
  while( GET_ADDR_BITS(aux_ptr) != GET_ADDR_BITS(s.target) )
  {
    tmp = *aux_ptr;
    --aux_ptr;
    _use(&tmp);
  }
  volatile char read_value[8];
  for (ssize_t i = 0; i < 8; i++)
  {
    read_value[i] = aux_ptr[i];
  }
  _use(read_value);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
