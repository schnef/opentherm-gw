#ifndef MANCHESTER_H_
#define MANCHESTER_H_

#include "data.h"

extern volatile Uint16_2x8_t t_min;
extern volatile Uint16_2x8_t t_max;
extern volatile Uint16_2x8_t t2_min;
extern volatile Uint16_2x8_t t2_max;

void manch_encode(volatile out_t *out, uint8_t out_mask, uint8_t timer_irq_mask);
void manch_decode(volatile in_t *in, uint16_t tc1_value);

#endif /* MANCHESTER_H_ */
