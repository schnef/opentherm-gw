#ifndef DATA_H_
#define DATA_H_

#include "constants.h"

typedef struct {
  uint8_t msg[FRAME_BYTES];
  uint8_t state;
  uint8_t buff;
  int8_t i;
  int8_t msg_bits_cntr;
  int8_t buff_bits_cntr;
  uint8_t parity;
  uint8_t prev_bit;
} in_t;

typedef struct {
  uint8_t msg[FRAME_BYTES];
  uint8_t state;
  uint8_t buff;
  int8_t i;
  int8_t msg_bits_cntr;
  int8_t buff_bits_cntr;
  uint8_t clock_half;
} out_t;

typedef union {
  uint16_t value;
  struct {
    uint8_t valuel;
    uint8_t valueh;
  };
} Uint16_2x8_t;


#endif /* DATA_H_ */
