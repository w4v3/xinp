/*
 * Here we process the command line arguments.
 *
 * When called with no arguments or --help, the
 * usage is displayed; otherwise, they are all
 * interpreted as file names.
 */

#include <stdio.h>
#include <string.h>

void print_usage();

int current_file_index = 0;

int has_next_file(int argc, char **argv) {
  if (argc == 1 || (argc == 2 && !strcmp(argv[1], "--help"))) {
    print_usage();
    return 0;
  }
  return current_file_index <= argc - 2;
}

char *next_file(int argc, char **argv) {
  current_file_index++;
  printf("%s\n", argv[current_file_index]);
  return argv[current_file_index];
}

void print_usage() {
  printf(
      "This is the xinp tool for extracting Dell Backup And Recovery files (*.inp).\n"
      "\n"
      "Usage: xinp file...\n"
      "\n"
      "The tool will try its best to extract the backed up files from the supplied list of inp files/folders containing those files.\n"
      "Only full backups are supported.\n"
      "\n"
      "Open an issue on <github.com/w4v3/xinp> to contact me.\n"
  );
}
