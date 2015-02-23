#include <avr/io.h>

#include "constants.h"
#include "data.h"
#include "manchester.h"

/*
 * De volgende macro's worden gebruikt bij het sturen en ontvangen van
 * bits bij de Manchester en/decode.  SENDBIT original data XOR clock
 * = Manchester value: zie
 * http://en.wikipedia.org/wiki/Manchester_code waarbij de waarde van
 * clock of 0x00 of 0xFF is en dus niet 0x00 en 0x01.  LET OP: output
 * is geinverteerd!! IDLE = hoog NEXT_CLK_HALF Toggle clock door XOR
 * te nemen met de waarde 1
 */
#define SENDBIT(value) if ((value ^ out->clock_half) & 0x80) {PORTD &= ~out_mask;} else {PORTD |= out_mask;}
#define NEXT_CLK_HALF(clk) (clk ^= ONE)

/*
 * Verstuur data door een aantal bits te coderen volgens Manchester
 * encoding.  out_mask geeft aan welke bits op de D poort gebruikt
 * worden = meestal maar een.  timer_irq_mask is de irq van de timer
 * die deze routine aanroept. Als alles verzonden is moet de timer
 * interrupt uitgezet worden.
 */
void manch_encode(volatile out_t *out, uint8_t out_mask, uint8_t timer_irq_mask) {
  switch (out->state) {
  case START:
    out->i = out->msg_bits_cntr = out->buff_bits_cntr = out->clock_half = ZERO;
    out->buff = out->msg[out->i];
    out->state = SEND_START_BIT;
    PORTB |= (1 << LED2);
    break;
  case SEND_START_BIT:
    SENDBIT(ONE);
    if (out->clock_half == ONE) {
      out->state = SEND_MSG;
    }
    NEXT_CLK_HALF(out->clock_half);
    break;
  case SEND_MSG:
    SENDBIT(out->buff);
    if (out->clock_half == ONE) {
      ++ out->msg_bits_cntr;
      if (out->msg_bits_cntr == FRAME_BITS) {
	out->state = SEND_STOP_BIT;
      } else {
	out->buff <<= 1;
	++ out->buff_bits_cntr;
	if (out->buff_bits_cntr == 8) {
	  ++ out->i;
	  out->buff = out->msg[out->i];
	  out->buff_bits_cntr = 0;
	}
      }
    }
    NEXT_CLK_HALF(out->clock_half);
    break;
  case SEND_STOP_BIT:
    SENDBIT(ONE);
    if (out->clock_half == ONE) {
      out->state = DONE;
    }
    NEXT_CLK_HALF(out->clock_half);
    break;
  case DONE:
    TIMSK &= ~timer_irq_mask;  // disable further interrupts
    PORTD |= out_mask; // set output to idle = HOOG!!
    out->state = IDLE;
    PORTB &= ~(1 << LED2);
    break;
  }
}

volatile Uint16_2x8_t t_min = { T_MIN };
volatile Uint16_2x8_t t_max = { T_MAX };
volatile Uint16_2x8_t t2_min = { T2_MIN };
volatile Uint16_2x8_t t2_max = { T2_MAX };

/*
 * Ontvang een bitstream en decodeer deze volgens Manchester.  TODO:
 * T_MIN en T_MAX kunnen variabel gemaakt worden waarbij het sync bit
 * wordt gebruikt om een korte periode te bepalen. Zie ook Atmel AVR
 * application notes: AVR410 and AVR415.  TODO: Het is mogelijk om in
 * deze routine bij te houden of het aantal ontvangen eenen even of
 * oneven is. Kan als return value worden teruggegeven waarna het
 * uitvoeren van een parity check niet meer nodig is.
 */
void manch_decode(volatile in_t *in, uint16_t tc1_value) {

#define UNDEFINED 0
#define SHORT 1
#define LONG 2

  uint8_t t_time = UNDEFINED;

  if (in->state != WAITING) {
    if ((tc1_value >= t_min.value) && (tc1_value <= t_max.value)) {
      t_time = SHORT;
    } else if ((tc1_value >= t2_min.value) && (tc1_value <= t2_max.value)) {
      t_time = LONG;
    } else {
      in->state = SYNC_ERROR;
    }
  }
  
  switch (in->state) {
  case WAITING:
    in->state = RCV_START_BIT;
    PORTB |= (1 << LED3);
    break;
  case RCV_START_BIT:  // Dit valt op overgang halverwege bit frame
    if (t_time == SHORT) {
      in->prev_bit = ONE;
      // TODO: Misschien hele message leegmaken? Niet nodig voor OpenTherm
      in->i = in->msg_bits_cntr = in->buff_bits_cntr = in->buff = in->parity = 0;
      in->state = RCV_MSG;
    } else {
      in->state = ERROR;
    }
    break;
  case RCV_MSG:
    if (t_time == LONG) {
      // Dit is 2T vanaf vorige overgang halverwege bitframe.
      in->prev_bit ^= ONE;
      in->state = STORE_BIT;
    } else if (t_time == SHORT) {
      // Dit is T vanaf vorige overgang = begin nieuw bitframe.
      in->state = RCV_MSG_2;
    } else {
      in->state = ERROR;
    }
    break;
  case RCV_MSG_2:
    if (t_time == SHORT) {
      // We zitten weer halverwege bitframe
      in->state = STORE_BIT;
    } else {
      in->state = ERROR;
    }
    break;
  case RCV_STOP_BIT:
    if (t_time == LONG) {
      in->state = DONE;
    } else if (t_time == SHORT) {
      in->state = RCV_STOP_BIT_2;
    } else {
      in->state = ERROR;
    }
    break;
  case RCV_STOP_BIT_2:
    if (t_time == SHORT) {
      in->state = DONE;
    } else {
      in->state = ERROR;
    }
    break;
  }

  switch (in->state) {
  case STORE_BIT:
    if (in->prev_bit) {
      in->buff |= 0x01;
      in->parity ^= ONE;
    }
    ++ in->buff_bits_cntr;
    ++ in->msg_bits_cntr;
    if (in->msg_bits_cntr == FRAME_BITS) {
      in->msg[in->i] = in->buff;
      in->state = RCV_STOP_BIT;
    } else {
      if (in->buff_bits_cntr == 8) {
	in->msg[in->i] = in->buff;
	++ in->i;
	in->buff_bits_cntr = 0;
	in->buff = 0;
      } else {
	in->buff <<= 1;
      }
      in->state = RCV_MSG;
    }
    break;
  case ERROR:
  case SYNC_ERROR:
    in->state = WAITING;
    break;
  case DONE:
    PORTB &= ~(1 << LED3);
    break;
  }
  //return in->parity;
}
