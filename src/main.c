#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

#include "file.h"
#include "common.h"
#include "parse.h"

int main(int argc, char *argv[]) {
  char* filepath = NULL;
  int c = 0;
  bool newFile = false;
  bool list = false;
  char* addstring = NULL;
  int dbfd = STATUS_ERROR;
  struct dbheader_t *dbhead = calloc(1, sizeof(struct dbheader_t));
  if (dbhead == NULL) {
    return STATUS_ERROR;
  }

  struct employee_t *empout = calloc(1, sizeof(struct employee_t));
  if (empout == NULL) {
    return STATUS_ERROR;
  }

  while ((c = getopt(argc, argv, "nf:a:l:")) != -1) {
    switch(c) {
      case 'f':
        filepath = optarg;
        break;
      case 'n':
        newFile = true;
        break;
      case 'a':
        addstring = optarg;
        break;
      case 'l':
        list = true;
        break;
      default:
        return STATUS_ERROR;
    }
  }

  if (newFile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("unable to create db file");
      return -1;
    }

    if (create_db_header(&dbhead) == STATUS_ERROR) {
      printf("failed to create db head");
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("unable to open db file");
      return -1;
    }

    if (validate_db_header(dbfd, &dbhead) == STATUS_ERROR) {
      printf("failed to validate db head");
      return -1;
    }
  }

  if (list) {
    list_employee(dbhead, &empout);
    return 1;
  }

  if (addstring) {
    add_employee(dbhead, empout, addstring);
  }

  printf("Newfile: %d\n", newFile);
  printf("Filepath: %s\n", filepath);

  read_employees(dbfd, dbhead, &empout);
  output_file(dbfd, dbhead, empout);

  return STATUS_SUCCESS;
}