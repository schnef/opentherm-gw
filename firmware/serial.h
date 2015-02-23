#ifndef SERIAL_H_
#define SERIAL_H_

#include "constants.h"

/* Circular buffer type */
typedef struct {
  uint8_t start;
  uint8_t end;
  uint8_t count;
  uint8_t buffer[BUF_SIZE];
} cb_t;

void uputc(uint8_t c);
void uputs(uint8_t *s);
uint8_t ugetc_nb(uint8_t *c);

#endif /* SERIAL_H_ */
