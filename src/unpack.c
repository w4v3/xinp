/*
 * Here we do the extraction of the LZ77-type
 * compressed files.
 *
 * The maximum lookback buffer is assumed to be 0x100
 * bytes long. A circular array is used for buffering,
 * and whenever a round is finished, it is flushed to disk.
 */

#include "entry.h"

// buffered reading and writing
FILE *read_file;
byte read_buffer[BUF_SIZE];
int read_buf_idx = BUF_SIZE;

FILE *write_file;
byte write_buffer[BUF_SIZE];
int write_buf_idx = 0;

byte read_next_byte() {
  if (read_buf_idx == BUF_SIZE) {
    fread(read_buffer, BUF_SIZE, 1, read_file);
    read_buf_idx = 0;
  }

  return read_buffer[read_buf_idx++];
}

void write_byte(byte data) {
  if (write_buf_idx == BUF_SIZE) {
    fwrite(write_buffer, BUF_SIZE, 1, write_file);
    write_buf_idx = 0;
  }

  write_buffer[write_buf_idx++] = data;
}

void flush() {
  fwrite(write_buffer, write_buf_idx, 1, write_file);
}

// circular buffer: if a distance greater than the current position
// is requested, continue from the end of the array
byte get_extracted(int distance) {
  if (distance >= write_buf_idx)
    return write_buffer[BUF_SIZE - (distance - write_buf_idx) - 1];
  else
    return write_buffer[write_buf_idx - distance - 1];
}

// processing is done using finite state machine
typedef enum {
  determine_next,
  determine_length,
  determine_distance,
  insert_reference,
  insert_literal
} STATE;

uint16_t insert_length;
uint16_t insert_distance;

STATE process(byte next, STATE state) {
  switch(state) {
    case determine_next: {
      // there are three cases, depending
      // on the first nibble:
      // - 0 or 1: literal follows
      // - 2-c: reference follows
      // - e: reference of extended length follows
      if (next < 0x20) {
        insert_length = next;
        return insert_literal;
      } else if (next < 0xe0) {
        insert_distance = (next & 0x0f) << 8;
        insert_length = (next >> 4) / 2 + 2;
        return determine_distance;
      } else {
        insert_distance = (next & 0x0f) << 8;
        insert_length = 9;
        return determine_length;
      }
    }
    case insert_literal: {
      write_byte(next);

      if (insert_length > 0) {
        insert_length--;
        return insert_literal;
      } else {
        return determine_next;
      }
    }
    case determine_distance: {
      insert_distance += next;
      return insert_reference;
    }
    case determine_length: {
      insert_length += next;
      return determine_distance;
    }
    case insert_reference: {
      write_byte(get_extracted(insert_distance));

      insert_length--;

      if (insert_length > 0) {
        return insert_reference;
      } else {
        insert_distance = 0;
        return determine_next;
      }
    }
  }

  return determine_next;
}

// we are right at the beginning of the file in the entry
void extract_entry(ENTRY entry) {
  read_file = entry.container;
  write_file = entry.write_to;

  read_buf_idx = BUF_SIZE;
  write_buf_idx = 0;

  STATE state = determine_next;

  for (int i = 0; i < entry.size; i++) {
    if (state != insert_reference)
      state = process(read_next_byte(), state);
    else {
      i--;
      state = process('\0', state);
    }
  }

  flush();
  fclose(entry.write_to);
  fclose(entry.container);
}
