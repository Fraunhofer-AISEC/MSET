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

char * aux_ptr;

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
  if ( !((ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin)) <= 0) ) _exit(PRECONDITIONS_FAILED_VALUE);
  if ( GET_ADDR_BITS(&aux_ptr) < GET_ADDR_BITS(origin) && GET_ADDR_BITS(&aux_ptr) > GET_ADDR_BITS(target) ) _exit(PRECONDITIONS_FAILED_VALUE);
  aux_ptr = origin;
  while( GET_ADDR_BITS(aux_ptr) != GET_ADDR_BITS(target) )
  {
    *aux_ptr = 0xFF;
    --aux_ptr;
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
