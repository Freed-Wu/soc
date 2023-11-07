/*
 * Test program to simulate center board
 * Refer docs/resources/serial-transmission-protocol.md
 *
 * Run:
 * socat pty,rawer,link=/tmp/ttyS0 pty,rawer,link=/tmp/ttyS1
 */
#include "master.h"
#include <fcntl.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wordexp.h>

static opt_t *parse(int argc, char *argv[]) {
  opt_t *p_opt = malloc(sizeof(opt_t));
  if (p_opt == NULL) {
    perror("p_opt");
    return NULL;
  }
  memcpy(p_opt, &default_opt, sizeof(opt_t));
  int c;
  char optstring[] = "t:i:o:";
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch (c) {
    case 't':
      p_opt->tty = optarg;
      break;
    case 'o':
      p_opt->output_dir = optarg;
      break;
    case 'i':
      p_opt->img_number++;
    }
  }
  if (p_opt->img_number == 0) {
    printf("usage: %s [-t TTY] [-o OUTPUT_DIR] -i IMAGE1 [-i IMAGE2 ...]\n",
           argv[0]);
    return NULL;
  }

  p_opt->imgs = malloc(p_opt->img_number * sizeof(img_t));
  int i = 0;
  wordexp_t exp_result;
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch (c) {
    case 'i':
      wordexp(optarg, &exp_result, 0);
      p_opt->imgs[i].name = exp_result.we_wordv[0];
      struct stat file_stat;
      if (stat(p_opt->imgs[i].name, &file_stat) == -1) {
        perror(p_opt->imgs[i].name);
        return NULL;
      }
      p_opt->imgs[i].size = file_stat.st_size;
      p_opt->imgs[i].file = malloc(p_opt->imgs[i].size);
      if (p_opt->imgs[i].file == NULL) {
        perror(p_opt->imgs[i].name);
        return NULL;
      }
      FILE *file = fopen(p_opt->imgs[i].name, "r");
      if (fread(p_opt->imgs[i].name, 1, p_opt->imgs[i].size, file) !=
          p_opt->imgs[i].size) {
        perror(p_opt->imgs[i].name);
        return NULL;
      }
      fclose(file);
      i++;
      break;
    }
  }
  return p_opt;
}

int main(int argc, char *argv[]) {
  opt_t *p_opt = parse(argc, argv);
  if (p_opt == NULL) {
    return EXIT_FAILURE;
  }
  int fd = open(p_opt->tty, O_RDWR | O_NONBLOCK | O_NOCTTY);
  if (fd == -1) {
    perror(p_opt->tty);
    return EXIT_FAILURE;
  }
  frame_t input_frame, output_frame = default_frame;

  for (;;) {
    wait_frame(fd, &input_frame, &output_frame);

    switch (input_frame.frame_type) {
    case TP_FRAME_TYPE_REQUEST_DATA:
      output_frame.frame_type = TP_FRAME_TYPE_TRANSPORT_DATA;
      output_frame.n_file = input_frame.n_file;
      output_frame.data_len = TP_FRAME_DATA_LEN_MAX;
      img_t img = p_opt->imgs[input_frame.n_file];
      size_t totol_frames = img.size / TP_FRAME_DATA_LEN_MAX +
                            !!(img.size % TP_FRAME_DATA_LEN_MAX);
      uint8_t *p_file = img.file;
      for (size_t i = 0; i < totol_frames; i++) {
        if (i == totol_frames - 1)
          output_frame.data_len = img.size % TP_FRAME_DATA_LEN_MAX;
        memcpy(output_frame.data, p_file, output_frame.data_len);
        p_file += output_frame.data_len;
        if (send_frame(fd, &output_frame) == -1) {
          perror(p_opt->tty);
          break;
        }
        printf("master: send data: raw image %d [%ld/%ld]\n",
               input_frame.n_file, i, totol_frames);
      }
      output_frame.frame_type = TP_FRAME_TYPE_REQUEST_DATA;
      printf("master: request data: compressed image %d\n", input_frame.n_file);
      if (send_frame(fd, &output_frame) == -1)
        perror(p_opt->tty);
      break;
    case TP_FRAME_TYPE_TRANSPORT_DATA:
      printf("master: receive data: compressed image %d\n", input_frame.n_file);
      char *name = basename(p_opt->imgs[input_frame.n_file].name);
      size_t len = strlen(p_opt->output_dir) + strlen(name) + strlen("/.bin");
      char *filename = malloc(len);
      if (sprintf(filename, "%s/%s.bin", p_opt->output_dir, name) != len) {
        perror(name);
        break;
      }
      FILE *file = fopen(filename, "a");
      if (file == NULL) {
        perror(filename);
        break;
      }
      if (fwrite(input_frame.data, 1, input_frame.data_len, file) !=
          input_frame.data_len) {
        perror(filename);
        break;
      }
      fclose(file);
    }
    output_frame.n_frame++;
  }
}
