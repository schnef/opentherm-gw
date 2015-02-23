
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "serial.h"

/*
 * Serieele I/O via de USART met bijvoorbeeld de Raspberry Pi.  De
 * serieele I/O loopt via een ringbuffer.
 */

volatile cb_t cb_in = { 0, 0, 0, {} };
volatile cb_t cb_out = { 0, 0, 0, {} };

/* BELANGRIJK: De grootte van de buffer moet een macht van twee zijn
 * omdat bij de 'wrap around' de modulo door een AND functie wordt
 * gebruikt en bij foute waarden de boel verschrikkelijk mis gaat.
 */

/* 
 * Schrijf een karakter in de buffer. De functie geeft het aantal
 * karakters terug die zich na het schrijven in de bugger bevinden of,
 * in het geval dat de buffer vol was, een 0. Functie is atomic en
 * interrupts worden dus afgehouden tijdens deze functie.
 */
uint8_t cb_putc(volatile cb_t *cb, uint8_t c) {
  uint8_t end, rv;
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (cb->count < BUF_SIZE) {
      // end = (cb->start + cb->count) % CIRC_BUF_SIZE;
      end = ((cb->start + cb->count) & (BUF_SIZE - 1));
      cb->buffer[end] = c;
      ++ cb->count;
      rv = cb->count;
    } else {
      rv = 0;
    }
  }
  return rv;
}

/* 
 * Lees het oudste karakter uit de buffer als die nog niet leeg is en
 * schrijf karakter naar de variabele waarnaar verwezen wordt en geef
 * de waarde 1 terug. Als de buffer leeg is, geef dan de waarde 0
 * terug. Functie wordt atomic uitgevoerd.
 */
uint8_t cb_getc(volatile cb_t *cb, uint8_t *c) {
  uint8_t rv;
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (cb->count > 0) {
      *c = cb->buffer[cb->start];
      //cb->start = (cb->start + 1) % CIRC_BUF_SIZE;
      cb->start = ((cb->start + 1) & (BUF_SIZE - 1));
      -- cb->count;
      rv = 1;
    } else {
      rv = 0;
    }
  }
  return rv;
}


// ============================== verzenden UART niveau ==============================

/*
 * Verzend een character vanuit de (ring)buffer via de USART.  Wacht
 * voor het verzenden tot het data register leeg is.
 */
void usart_putc(void) {
  uint8_t c;
  
  if (cb_getc(&cb_out, &c)) {
    while (!(UCSRA & (1<<UDRE))) {};
    UDR = c;
  }
}

/*
 * Verstuur character c.  Als de (ring)buffer vol is, wacht tot er
 * weerr plaats is en plaats character c in buffer.  Als dit het
 * eerste character in de buffer is, plaats het character dan ook in
 * het data register van de USART en enable de dataregister empty
 * interrupt.
 */
void uputc(uint8_t c) {
  while (cb_out.count == BUF_SIZE) {}
  if (cb_putc(&cb_out, (uint8_t) c) == 1) {
    usart_putc();
    UCSRB |= (1 << UDRIE);
  }
}

/*
 * Interrupt khandler voor data register uit van USART is
 * leeg. D.w.z. deze interrupt vuurt als een character is
 * verzonden. Ga na of er nog meer data moet worden verzonden.  Indien
 * dat niet het geval is, disable deze interrupt dan.
 */
ISR(USART_UDRE_vect) {
  uint8_t c;
  
  if (cb_getc(&cb_out, &c)) {
    UDR = c;
  } else {
    UCSRB &= ~(1 << UDRIE);
  }
}

// ============================== ontvangen UART niveau ==============================

/*
 * Non-boocking ontvangst. Als er wat voor handen is, kopieer
 * ontvangen karakter in variabele en geef een 1 terug, anders komt de
 * fucntie terug met 0
 */
uint8_t ugetc_nb(uint8_t *c) {
  
  //  if (!cb_is_empty(&cb_in)) {
  if (cb_in.count != 0) {
    cb_getc(&cb_in, c);
    return 1;
  } else {
    return 0;
  }
}

/*
 * Character ontvangen interrupt. Plaats character in (ring)buffer als
 * er plaats is.
 */
ISR(USART_RX_vect) {
  uint8_t c;
  
  c = UDR;
  cb_putc(&cb_in, c);
}


