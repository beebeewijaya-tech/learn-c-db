#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "common.h"
#include "parse.h"

static int read_full(int fd, void *buffer, size_t size) {
  size_t total = 0;
  unsigned char *dst = buffer;

  while (total < size) {
    ssize_t rc = read(fd, dst + total, size - total);
    if (rc == 0) {
      break;
    }
    if (rc < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("read");
      return STATUS_ERROR;
    }
    total += (size_t)rc;
  }

  if (total != size) {
    fprintf(stderr, "short read (%zu/%zu)\n", total, size);
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static int write_full(int fd, const void *buffer, size_t size) {
  size_t total = 0;
  const unsigned char *src = buffer;

  while (total < size) {
    ssize_t rc = write(fd, src + total, size - total);
    if (rc < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("write");
      return STATUS_ERROR;
    }
    total += (size_t)rc;
  }

  return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t **headerOut) {
  if (headerOut == NULL) {
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    perror("calloc");
    return STATUS_ERROR;
  }

  header->magic = HEADER_MAGIC;
  header->version = 0x1;
  header->count = 0;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;
  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0 || headerOut == NULL) {
    return STATUS_ERROR;
  }

  if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
    perror("lseek");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    perror("calloc");
    return STATUS_ERROR;
  }

  if (read_full(fd, header, sizeof(struct dbheader_t)) == STATUS_ERROR) {
    free(header);
    return STATUS_ERROR;
  }

  header->magic = ntohl(header->magic);
  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->filesize = ntohl(header->filesize);

  if (header->magic != HEADER_MAGIC) {
    fprintf(stderr, "magic not matched\n");
    free(header);
    return STATUS_ERROR;
  }

  if (header->version != 1) {
    fprintf(stderr, "version not matched\n");
    free(header);
    return STATUS_ERROR;
  }

  struct stat sb = {0};
  if (fstat(fd, &sb) == -1) {
    perror("fstat");
    free(header);
    return STATUS_ERROR;
  }

  size_t headerSize = sizeof(struct dbheader_t);
  if (header->filesize < headerSize) {
    fprintf(stderr, "invalid header filesize\n");
    free(header);
    return STATUS_ERROR;
  }

  if ((off_t)header->filesize > sb.st_size) {
    fprintf(stderr, "header filesize larger than actual file\n");
    free(header);
    return STATUS_ERROR;
  }

  size_t payloadSize = header->filesize - headerSize;
  size_t requiredPayload = (size_t)header->count * sizeof(struct employee_t);
  if (payloadSize < requiredPayload) {
    fprintf(stderr, "header count larger than payload\n");
    free(header);
    return STATUS_ERROR;
  }

  *headerOut = header;
  return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *header, struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("got a bad fd!\n");
    return STATUS_ERROR;
  }

  int count = header->count;
  struct employee_t *employees = calloc(1, sizeof(struct employee_t));
  if (employees == NULL || employees == -1) {
    printf("calloc failed\n");
    return STATUS_ERROR;
  }

  read(fd, employees, count*sizeof(struct employee_t));
  int i = 0;
  for(; i < count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }
  *employeesOut = employees;
  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *header, struct employee_t *employees) {
  if (fd < 0 || header == NULL) {
    return STATUS_ERROR;
  }

  size_t expectedSize = sizeof(struct dbheader_t) + (size_t)header->count * sizeof(struct employee_t);
  header->filesize = (unsigned int)expectedSize;

  if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
    perror("lseek");
    return STATUS_ERROR;
  }

  struct dbheader_t headerOut = *header;
  headerOut.magic = htonl(headerOut.magic);
  headerOut.version = htons(headerOut.version);
  headerOut.count = htons(headerOut.count);
  headerOut.filesize = htonl(headerOut.filesize);

  if (write_full(fd, &headerOut, sizeof(headerOut)) == STATUS_ERROR) {
    return STATUS_ERROR;
  }

  if (header->count == 0) {
    return STATUS_SUCCESS;
  }

  if (employees == NULL) {
    fprintf(stderr, "missing employee data for %u entries\n", header->count);
    return STATUS_ERROR;
  }

  for (unsigned short i = 0; i < header->count; ++i) {
    struct employee_t record = employees[i];
    record.hours = htonl(record.hours);
    if (write_full(fd, &record, sizeof(record)) == STATUS_ERROR) {
      return STATUS_ERROR;
    }
  }

  return STATUS_SUCCESS;
}

int add_employee(struct dbheader_t *dbhead, struct employee_t *employees, char* addstring) {
  printf("%s\n", addstring);
  char *name = strtok(addstring, ",");
  char *addr = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");

  printf("%s %s %s", name, addr, hours);


  dbhead->count += 1;
  strncpy(employees[dbhead->count-1] .name, name, sizeof(employees[dbhead->count].name));
  strncpy(employees[dbhead->count-1] .address, addr, sizeof(employees[dbhead->count].address));
  employees[dbhead->count-1].hours = atoi(hours);
  dbhead->filesize = sizeof(struct dbheader_t) + (size_t)dbhead->count * sizeof(struct employee_t);

  return STATUS_SUCCESS;
}

void list_employee(struct dbheader_t *dbhead, struct employee_t *employees) {
  int i = 0;
  for (; i < dbhead->count; i++) {
    printf("employee: %d\n", i);
    printf("name: %s\n", employees[i].name);
    printf("addr: %s\n", employees[i].address);
    printf("------------\n");
  }
}