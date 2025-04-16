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
  char origin[8];
  char target[8];
};

// globals

struct T s = { {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB} };
char * aux_ptr = 0;

int f()
{
  // locals


  _use(s.target);
  _use(s.origin);
  if ( !((ssize_t)(GET_ADDR_BITS(s.target) - GET_ADDR_BITS(s.origin)) >= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  if ( GET_ADDR_BITS(&aux_ptr) < GET_ADDR_BITS(s.target) && GET_ADDR_BITS(&aux_ptr) > GET_ADDR_BITS(s.origin) ) _exit(PRECONDITIONS_FAILED_VALUE);
  aux_ptr = s.origin;
  while( GET_ADDR_BITS(aux_ptr) != GET_ADDR_BITS(s.target) )
  {
    *aux_ptr = 0xFF;
    ++aux_ptr;
    _use(aux_ptr);
  }
  
  for (ssize_t i = 0; i < 8; i++)
  {
    aux_ptr[i] = 0xFF;
  }
  _use(aux_ptr);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  return 0;
}

int main()
{
  f();

  return 0;
}
