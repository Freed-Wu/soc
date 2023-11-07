#include "main.h"
#include "coding.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static opt_t *parse(int argc, char *argv[]) {
  opt_t *p_opt = malloc(sizeof(opt_t));
  if (p_opt == NULL) {
    perror("p_opt");
    return NULL;
  }
  memcpy(p_opt, &default_opt, sizeof(opt_t));
  int c;
  char optstring[] = "t:o:";
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch (c) {
    case 't':
      p_opt->tty = optarg;
      break;
    case 'o':
      p_opt->output_dir = optarg;
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
  send_frame(fd, &output_frame);
  printf("slave: request data: raw image 0\n");
  for (;;) {
    wait_frame(fd, &input_frame, &output_frame);

    switch (input_frame.frame_type) {
    case TP_FRAME_TYPE_CONTROL:
      /*! TODO: not implemented yet
       *  \todo not implemented yet
       */
      break;
    case TP_FRAME_TYPE_REQUEST_DATA:
      output_frame.frame_type = TP_FRAME_TYPE_TRANSPORT_DATA;
      output_frame.n_file = input_frame.n_file;
      output_frame.data_len = TP_FRAME_DATA_LEN_MAX;
      /*! TODO: Create a thread to compress raw image and send a frame after
       * finishing \todo Create a thread to compress raw image and send a frame
       * after finishing
       */
      break;
    case TP_FRAME_TYPE_TRANSPORT_DATA:
      output_frame.frame_type = TP_FRAME_TYPE_TRANSPORT_DATA;
      printf("slave: receive data: raw image %d\n", input_frame.n_file);
      size_t len = strlen(p_opt->output_dir) + 4 + strlen("/.yuv");
      char *filename = malloc(len);
      if (sprintf(filename, "%s/%d.yuv", p_opt->output_dir,
                  input_frame.n_file) != len) {
        perror(p_opt->output_dir);
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
