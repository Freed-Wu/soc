#include "../src/transmission_protocol.h"
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

/*
 * check_sum doesn't need to be asserted.
 */
void is_same_frame(frame_t frame1, frame_t frame2) {
  EXPECT_EQ(std::vector<uint8_t>(frame1.header,
                                 frame1.header + sizeof(frame1.header)),
            std::vector<uint8_t>(frame2.header,
                                 frame2.header + sizeof(frame2.header)));
  EXPECT_EQ(frame1.address, frame2.address);
  EXPECT_EQ(frame1.frame_type, frame2.frame_type);
  EXPECT_EQ(frame1.n_file, frame2.n_file);
  EXPECT_EQ(frame1.cmd_id, frame2.cmd_id);
  EXPECT_EQ(frame1.n_frame, frame2.n_frame);
  EXPECT_EQ(frame1.data_len, frame2.data_len);
  EXPECT_EQ(std::vector<uint8_t>(frame1.data, frame1.data + frame1.data_len),
            std::vector<uint8_t>(frame2.data, frame2.data + frame2.data_len));
}

TEST(transmission_protocol, frame2bit_stream) {
  uint8_t expected[] = "\xEB\x90\xEB\x90\x1\0\x3\0\x1\0\0\0*\0\x14\xFD";
  frame_t frame;
  memcpy(frame.header, tp_header, sizeof(tp_header));
  frame.address = TP_ADDRESS_SLAVE;
  frame.frame_type = TP_FRAME_TYPE_REQUEST_DATA;
  frame.n_file = 1;
  frame.n_frame = 42;
  uint8_t *bit_stream = (uint8_t *)malloc(sizeof(expected) - 1);
  frame2bit_stream(frame, bit_stream);
  EXPECT_EQ(std::vector<uint8_t>(bit_stream, bit_stream + sizeof(expected) - 1),
            std::vector<uint8_t>(expected, expected + sizeof(expected) - 1));
  frame_t *p_frame = bit_stream2frame(bit_stream);
  is_same_frame(*p_frame, frame);
}

TEST(transmission_protocol, send_frame) {
  FILE *file0 = fopen("/tmp/ttyS0", "r"), *file1 = fopen("/tmp/ttyS1", "r");
  if (file0 == NULL && file1 == NULL) {
    printf("please run 'socat pty,link=/tmp/ttyS0,rawer "
           "pty,link=/tmp/ttyS1,rawer'\n");
    return;
  }
  int fd = open("/tmp/ttyS0", O_RDWR | O_NONBLOCK | O_NOCTTY);
  frame_t frame;
  memcpy(frame.header, tp_header, sizeof(tp_header));
  frame.address = TP_ADDRESS_SLAVE;
  frame.frame_type = TP_FRAME_TYPE_REQUEST_DATA;
  frame.n_file = 1;
  frame.n_frame = 42;
  EXPECT_NE(send_frame(fd, &frame), -1);
  close(fd);
  fd = open("/tmp/ttyS1", O_RDWR | O_NONBLOCK | O_NOCTTY);
  frame_t result;
  EXPECT_GT(receive_frame(fd, &result), 0);
  is_same_frame(frame, result);
}
