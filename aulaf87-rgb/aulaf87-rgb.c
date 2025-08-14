// 2025 Artem Lukyanov
// Docs: https://github.com/NollieL/SignalRgb_CN_Key

#include <errno.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_PATH "/var/run/aulaf87-rgb.fifo"
#define HID_VENDOR_ID 0x258a
#define HID_PRODUCT_ID 0x010c
#define VKEYS_LENGTH sizeof(vKeys) / sizeof(vKeys[0])
#define BUFFER_SIZE 520
#define REPORT_FUNCTION(x) (uint8_t)((x >> 4) & 0b00001111)
#define REPORT_TYPE(x) (uint8_t)((x >> 2) & 0b00000011)
#define REPORT_LENGTH(x) (uint8_t)(x & 0b00000011)
#define REPORT_USAGE_PAGE(x) REPORT_TYPE(x) == 0x01 && REPORT_FUNCTION(x) == 0x00
#define REPORT_USAGE(x) REPORT_TYPE(x) == 0x02 && REPORT_FUNCTION(x) == 0x00

const int vKeys[] = {0,  12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90,
                     96, 1,  7,  13, 19, 25, 31, 37, 43, 49, 55, 61, 67, 73, 79,
                     85, 91, 97, 2,  8,  14, 20, 26, 32, 38, 44, 50, 56, 62, 68,
                     74, 80, 86, 92, 98, 3,  9,  15, 21, 27, 33, 39, 45, 51, 57,
                     63, 69, 81, 4,  10, 16, 22, 28, 34, 40, 46, 52, 58, 64, 82,
                     94, 5,  11, 17, 35, 53, 59, 65, 83, 89, 95, 101};

char buf[BUFFER_SIZE];

void *reader_process(void *vargp) {
  FILE * fp;
  size_t len = 0;
  ssize_t read;
  int i, iLedIdx, res, fd, key = -1, r = 0, g = 0, b = 0;
  char *pch, *line = NULL;

  while (1) {
    fp = fopen(FIFO_PATH, "r");
    if (fp == NULL)
      exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
      pch = strtok(line, " \n");
      if (pch == NULL) {
        goto fd;
      }
      key = strtol(pch, NULL, 10);
      pch = strtok(NULL, " \n");
      if (pch == NULL) {
        goto fd;
      }
      sscanf(pch, "%02x%02x%02x", &r, &g, &b);

      if (key > 0 && key <= VKEYS_LENGTH) {
        iLedIdx = vKeys[key - 1] * 3;
        buf[iLedIdx + 8] = r;
        buf[iLedIdx + 1 + 8] = g;
        buf[iLedIdx + 2 + 8] = b;
      } else if (key == 255) {
        for (i = 0; i < VKEYS_LENGTH; i++) {
          iLedIdx = vKeys[i] * 3;
          buf[iLedIdx + 8] = r;
          buf[iLedIdx + 1 + 8] = g;
          buf[iLedIdx + 2 + 8] = b;
        }
      }
    }
    if (line) {
      free(line);
      line = NULL;
    }
fd:
    fclose(fp);
  }
}

int main(int argc, char *argv[]) {
  int fd, res, desc_size, i;
  struct hidraw_devinfo info;
  struct hidraw_report_descriptor rpt_desc;
  pthread_t tid;

  if (argc != 2) {
    printf("path to hidraw device not given\n");
    return 1;
  }

  fd = open(argv[1], O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    perror("Unable to open device");
    return 1;
  }

  res = ioctl(fd, HIDIOCGRAWINFO, &info);
  if (res < 0) {
    perror("HIDIOCGRAWINFO");
    return 1;
  }
  if (info.vendor != HID_VENDOR_ID || info.product != HID_PRODUCT_ID) {
    printf("wrong device\n");
    return 1;
  }

  desc_size = 0;
  res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
  if (res < 0) {
    perror("HIDIOCGRDESCSIZE");
    return 1;
  }

  memset(&rpt_desc, 0x0, sizeof(rpt_desc));
  rpt_desc.size = desc_size;
  res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
  if (res < 0) {
    perror("HIDIOCGRDESC");
    return 1;
  }

  i = 0, res = -1;
  while (i < rpt_desc.size) {
    if (REPORT_USAGE_PAGE(rpt_desc.value[i]) && REPORT_LENGTH(rpt_desc.value[i]) == 2 &&
        rpt_desc.value[i + 1] == 0x00 && rpt_desc.value[i + 2] == 0xff) {
      i = i + 1 + REPORT_LENGTH(rpt_desc.value[i]);
      break;
    }
    i = i + 1 + REPORT_LENGTH(rpt_desc.value[i]);
  }
  while (i < rpt_desc.size) {
    if (REPORT_USAGE(rpt_desc.value[i]) && REPORT_LENGTH(rpt_desc.value[i]) == 1 &&
        rpt_desc.value[i + 1] == 0x01) {
      res = 0;
      break;
    }
    i = i + 1 + REPORT_LENGTH(rpt_desc.value[i]);
  }
  if (res < 0) {
    printf("wrong device\n");
    return 1;
  }

  memset(buf, 0x0, BUFFER_SIZE);
  buf[0] = 0x06;
  buf[1] = 0x08;
  buf[2] = 0x00;
  buf[3] = 0x00;
  buf[4] = 0x01;
  buf[5] = 0x00;
  buf[6] = 0x7a;
  buf[7] = 0x01;

  umask(0);
  mkfifo(FIFO_PATH, S_IWUSR | S_IWGRP | S_IWOTH);

  pthread_create(&tid, NULL, reader_process, NULL);

  while (1) {
    res = ioctl(fd, HIDIOCSFEATURE(BUFFER_SIZE), buf);
    if (res < 0) {
      perror("HIDIOCSFEATURE");
      return 1;
    }
    usleep(100000);
  }

  return 1;
}
