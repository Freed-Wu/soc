#include "crc.h"

/*
 * https://github.com/stephane/libmodbus/blob/b25629bfb508bdce7d519884c0fa9810b7d98d44/src/modbus-rtu.c#L116
 */
uint16_t crc16(uint8_t *buffer, uint16_t buffer_length) {
  uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
  uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
  unsigned int i;        /* will index into CRC lookup */

  /* pass through message buffer */
  while (buffer_length--) {
    i = crc_lo ^ *buffer++; /* calculate the CRC  */
    crc_lo = crc_hi ^ table_crc_hi[i];
    crc_hi = table_crc_lo[i];
  }

  return (crc_hi << 8 | crc_lo);
}
