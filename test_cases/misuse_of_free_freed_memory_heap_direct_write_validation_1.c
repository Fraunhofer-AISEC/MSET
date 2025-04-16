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

char* heap_obj;

int f()
{
  // locals


  char *target = (char *)malloc( 160 );
  
  #ifndef __GLIBC__
  exit(PRECONDITIONS_FAILED_VALUE); // not using glibc
  #endif
  for (size_t i = 0; i < 16; i++) target[i] = 0;
  target[8] = 0x40; // magic value
  target[13*8] = 0x40;
  unsigned long *crafted_ptr;
  crafted_ptr = (unsigned long *)&(target[2*8]); // pointing at byte 0x10, content of chunk 0
  (void)malloc(8);
  (void)crafted_ptr;
  
  heap_obj = (char *)malloc(8);

  free(target);
  
  for (ssize_t i = 0; i < 8; i++)
  {
    heap_obj[i] = 0xFF;
  }
  _use(heap_obj);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);
  return 0;
}

int main()
{
  f();

  return 0;
}
