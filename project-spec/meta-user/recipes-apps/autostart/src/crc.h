/**
 * ./crc_c_generator.py --fn=crc16 --ro --iv=0xffff 16 12 5 0 crc.h crc.c
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t crc16(const uint8_t *buffer, size_t count);

#ifdef __cplusplus
} /* extern "C" */
#endif
