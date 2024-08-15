/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_UTILS_H_
#define OCRE_UTILS_H

#include <stdint.h>

/**
 * Converts a length-64 array of characters representing a sha256 into
 * a length-32 binary representation byte array .
 */
int sha256str_to_bytes(uint8_t *bytes, const uint8_t *str) {
  uint8_t tmp;
  for (int i = 0; i < 64; i++) {
    if (str[i] >= '0' && str[i] <= '9') {
      tmp = str[i] - '0';
    } else if (str[i] >= 'a' && str[i] <= 'z') {
      tmp = str[i] - 'a' + 10;
    } else if (str[i] >= 'A' && str[i] <= 'Z') {
      tmp = str[i] - 'A' + 10;
    } else {
      return -1;
    }

    if (i % 2 == 0) {
      bytes[i / 2] = tmp << 4;
    } else {
      bytes[i / 2] |= tmp;
    }
  }

  return 0;
}

/**
 * Converts a length-32 byte array of a sha256 binary representaton
 * into a 64-byte length (plus null-terminator) string.
 */
int sha256bytes_to_str(const uint8_t *bytes, uint8_t *str) {
  uint8_t tmp[2];
  for (int i = 0; i < 32; i++) {
    tmp[0] = (bytes[i] & 0xF0) >> 4;
    tmp[1] = (bytes[i] & 0x0F);

    for (int j = 0; j < 2; j++) {
      if (tmp[j] >= 0x0 && tmp[j] <= 0x9) {
        tmp[j] += '0';
      } else if (tmp[j] >= 0xA && tmp[j] <= 0xF) {
        tmp[j] += ('a' - 0xA);
      } else {
        return -1;
      }
    }

    str[2 * i] = tmp[0];
    str[2 * i + 1] = tmp[1];
  }

  // Null terminator
  str[64] = '\0';

  return 0;
}

#endif