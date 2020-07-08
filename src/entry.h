#ifndef ENTRY_HEADER
#define ENTRY_HEADER
#include <stdio.h>
#include <stdint.h>

typedef unsigned char byte;
#define BUF_SIZE 0x2000

typedef struct {
  FILE *write_to;
  FILE *container;
  uint64_t size;
} ENTRY;
#endif
