#include <stdio.h>

#include "args.h"
#include "nav.h"
#include "unpack.h"

int main(int argc, char **argv) {
  while (has_next_file(argc, argv)) {
    char *filename = next_file(argc, argv);

    if (load_file(filename)) {
      while (has_next_entry()) {
        ENTRY entry = next_entry(filename);

        if (entry.size) {
          extract_entry(entry);
        }
      }
    }
  }
}
