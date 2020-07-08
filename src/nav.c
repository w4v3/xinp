/*
 * Here we find the right *FFB.inp file,
 * read through its entries and turn them
 * into an ENTRY struct.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "entry.h"

#define SIG_LENGTH 0xc0 // myterious signature at beginning of file
#define ENTRY_MAGIC_LENGTH 20 // these separate individual entries
static const byte ENTRY_MAGIC[ENTRY_MAGIC_LENGTH] = {
  0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 
  0x01, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
};
#define SHORT_PATH_LENGTH 0x1c // length of abbreviated filename

#if defined(WIN32) || defined(_WIN32) 
#define PATH_SEPARATOR '\\' 
#else 
#define PATH_SEPARATOR '/' 
#endif 

FILE *current_file;
byte buffer[BUF_SIZE];
int buf_idx = BUF_SIZE;

// this needs to be called first to initialize
int load_file(char *file) {
  current_file = fopen(file, "rb");

  if (!current_file) {
    printf("Error: File %s not found\n", file);
    return 0;
  }

  fseek(current_file, SIG_LENGTH, SEEK_SET);
  buf_idx = BUF_SIZE;

  return 1;
}

// buffered reading; signed short so that we can transmit EOF
signed short next_byte() {
  if (buf_idx == BUF_SIZE) {
    if (!fread(buffer, BUF_SIZE, 1, current_file))
      return EOF;
    buf_idx = 0;
  }

  return buffer[buf_idx++];
}

void skip_bytes(int n) {
  for (int i = 0; i < n; i++)
    next_byte();
}

uint64_t read_little_endian(int num_bytes) {
  uint64_t result = 0;

  for (int i = 0; i < num_bytes; i++)
    result = next_byte() << 8 * i | result;

  return result;
}

// literal string characters are interspersed with nulls
void read_literal_string(char *into, int length) {
  for (int i = 0; i < length; i++) {
    char nxt = next_byte();
    /*
    if (nxt == '\\') {
      nxt = PATH_SEPARATOR;
    }
    */
    into[i] = nxt;
    next_byte();
  }
}

// try to find the magic sequence for a new entry
int has_next_entry() {
  int matched = 0;

  for (signed short nxt = next_byte(); nxt != EOF; nxt = next_byte()) {
    if (nxt == ENTRY_MAGIC[matched]) {
      matched++;

      if (matched == ENTRY_MAGIC_LENGTH) {
        return 1;
      }
    } else {
      matched = 0;
    }
  }

  fclose(current_file);
  return 0;
}

/* entry layout:
 * 8 bytes for the length of the file
 * 8 bytes (unknown meaning)
 * 8 bytes for the offset of the file
 * 4 bytes for the index of the containing file
 * 28 bytes for the file name alias
 * 4 bytes for the length of the file path
 * the file path
 * some signature
 */
ENTRY next_entry(char *filename) {
  uint64_t file_size = read_little_endian(8);

  skip_bytes(8);

  uint64_t offset = read_little_endian(8);

  uint32_t container_id = read_little_endian(4);

  skip_bytes(SHORT_PATH_LENGTH);

  uint32_t path_length = read_little_endian(4) - 2; // path is off by 2 bytes

  char path[path_length + 1];
  read_literal_string(path, path_length);
  path[path_length] = '\0';

  // take the filename and append volume number to it
  // to arrive at the container file
  char container_path[strlen(filename) + 6];
  sprintf(container_path, "%.*s_%04d.inp", (int) strlen(filename) - 4, filename, container_id);
  FILE *container_file = fopen(container_path, "rb");

  if (!container_file) {
    printf("Error %d: file %s referenced but not found in folder.\n", errno, container_path);
    ENTRY result = { NULL, NULL, 0 }; // file size includes length
    printf("hi\n");
    return result;
  }

  fseek(container_file, offset, SEEK_SET);

  // first 4 bytes are header length in little endian, use to skip header
  byte header_length[4];
  fread(header_length, 4, 1, container_file);
  uint32_t skip = header_length[3] << 24 | header_length[2] << 16 | header_length[1] << 8 | header_length[0];
  fseek(container_file, skip + 4, SEEK_CUR); // 4 length bytes

  FILE *write_to = fopen(path, "wb");

  if (!write_to) {
    printf("Error %d: cannot write to file %s\n", errno, path);
  }

  ENTRY result = { write_to, container_file, file_size }; // file size includes length
  return result;
}
