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


int f()
{
  // locals


  struct T *s = (struct T *)malloc( sizeof(struct T) );
  s->target[0] = 0xAA;
  s->target[1] = 0xAA;
  s->target[2] = 0xAA;
  s->target[3] = 0xAA;
  s->target[4] = 0xAA;
  s->target[5] = 0xAA;
  s->target[6] = 0xAA;
  s->target[7] = 0xAA;
  s->origin[0] = 0xBB;
  s->origin[1] = 0xBB;
  s->origin[2] = 0xBB;
  s->origin[3] = 0xBB;
  s->origin[4] = 0xBB;
  s->origin[5] = 0xBB;
  s->origin[6] = 0xBB;
  s->origin[7] = 0xBB;
  volatile uint8_t read_value;
  read_value = *((volatile uint8_t *)(s->origin + (4 - 1)));
  _use(&read_value);
  _exit(TEST_CASE_SUCCESSFUL_VALUE);

  free(s);
  return 0;
}

int main()
{
  f();

  return 0;
}
