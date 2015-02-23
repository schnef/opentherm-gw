
#define F_CPU 11059200UL  // 11.052 MHz

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "constants.h"
#include "data.h"
#include "protocol.h"
#include "serial.h"
#include "manchester.h"

volatile uint8_t mode = PASSTHRU;

volatile out_t out_to_therm;
volatile out_t out_to_boiler;
volatile in_t in_from_therm;
volatile in_t in_from_boiler;

int main(void)  __attribute__((noreturn));

void init(void) {

  // Outputs.
  DDRD |= ((1 << TO_BOILER) | (1 << TO_THERM) | (1 << LED));
  PORTD |= ((1 << TO_BOILER) | (1 << TO_THERM));		// IDLE = HOOG

  DDRB |= ((1 << LED1) | (1 << LED2) | (1 << LED3));

  // Inputs, zonder pull-up = default van chip
  //DDR &= ~((1 << FROM_BOILER) | (1 << FROM_THERM));

  // INT0 en INT1 triggeren op verandering van logisch niveau.
  MCUCR |= ((0 << ISC11) | (1 << ISC10) | (0 << ISC01) | (1 << ISC00));
  GIMSK |= ((1 << INT1) | (1 << INT0));

  // Timer 0, wordt gebruikt als clock voor verzenden data.
  /*
   * Set timer/Counter 1 in mode 2 = Clear Timer on Compare
   * Match (CTC) Mode. Also, set the output compare register to
   * 86, which is half a clock cycle (500 uS) at 11.059200 MHz
   * with the prescaler set to 64, which gives an error of
   * approximatly 0.02%.
   */
  TCCR0A = (1 << WGM01); // CTC
  TCCR0B |= ((0 << CS02) | (1 << CS01) | (1 << CS00)); // prescaler set to 64
  OCR0A = OCR0B = T_1MS_8BIT; // 500 uS 

  // Timer 1, wordt gebruikt als clock voor ontvangen data.
  /*
   * De duur tussen overgangen van een ingang worden gemeten en
   * zijn input voor de Manchester decoding. Ook wordt er een
   * timeout gegenereerd als er 2 ms geen verandering op de
   * ingang is geweest, wat duidt op foute synchronisatie met
   * het signaal.
   */
  TCCR1B |= ((0 << CS12) | (1 << CS11) | (0 << CS10)); // prescaler set to 8
  OCR1A = T_1MS * 2; // timeout = signaal out of sync
  TIMSK |= (1 << OCIE1A); // enable timeout interrupt

  /*
   * Initialiseer de UART. Met de Raspberry pi en een 12MHz
   * kristal kan een baudrate tot 1Mb/s worden gehaald. Wel de
   * Raspberry pi bij booten benodige vlaggen meegeven om die
   * snelheid te halen. Ben er nog niet uit of het nu juist
   * voordeliger is om heel hoge UART snelheid te pakken of
   * niet. Bij hoge snelheid komen interrupts heel snel na
   * elkaar en zou de tijdmeting op de ingangen mischien nadelig
   * kunnen worden beinvloed. Gemiddeld moeten 8 karakters per
   * seconde verstouwd kunnen worden om de buffers niet vol te
   * laten lopen, en dat is heel erg langzaam dus.
   */
  // Standaard manier om baudrate in te stellen. 
#include <util/setbaud.h>
  UBRRH = UBRRH_VALUE;
  UBRRL = UBRRL_VALUE;
#if USE_2X
  UCSRA |= (1 << U2X);
#else
  UCSRA &= ~(1 << U2X);
#endif
  UCSRC |= ((0 << USBS) | (0 << UCSZ2) | (1 << UCSZ1) | (1 << UCSZ0));  // Asynchron 8N1
  UCSRB |= ((1 << RXEN) | (1 << TXEN) | (1 << RXCIE));  // UART RX, TX and RX Interrupt enable
}


/*
 * http://www-graphics.stanford.edu/~seander/bithacks.html#ParityParallel
 * 

unsigned int v; // word value to compute the parity of 
 v ^= v >> 16; 
 v ^= v >> 8; 
 v ^= v >> 4; 
 v &= 0xf; 
return (0x6996 >> v) & 1;

 * TODO: Optimaliseer de laatste shifts omdat dit door de compiler
 * toch nog omgezet wordt naar een loop. Ook zou de aanroep mooier
 * kunnen met een pointer naar de array en lokale variabelen.
 */
uint8_t parity32(volatile uint8_t *bytes) {
  uint8_t byte3 = bytes[2];
  uint8_t byte4 = bytes[3];
  byte3 ^= bytes[0];
  byte4 ^= bytes[1];
  byte4 ^= byte3;
  byte4 ^= byte4 >> 4;
  byte4 &= 0x0F;
  return (0x6996 >> byte4) & 1;
}

// ============================== zenden ==============================

/*
 * Bij het versturen van een bericht wordt eerst het bericht
 * gekopieerd naar een buffer van waaruit verstuurd wordt. Hierna
 * wordt nog de parity berekend en in de buffer gezet waarna de timer
 * wordt gestart. Het eigenlijke versturen wordt gedaan door de
 * interrupt handler die elke halve clock tijd door de timer wordt
 * gevuurd. Voor MASTER -> SLAVE wordt timer A gebruikt en voor SLAVE
 * -> MASTER wordt timer B gebruikt.
 */
void send(uint8_t *msg) {
  uint8_t dir;
  volatile out_t* out;

  // bit 6 = 0 == Master to slave == thermostaat naar ketel
  dir = msg[0] & (1 << MSTR_TO_SLV_BIT);
  if (dir) {
    out = &out_to_therm;
  } else {
    out = &out_to_boiler;
  }
  for (uint8_t i = 0; i < FRAME_BYTES; i++) {
    out->msg[i] = msg[i];
  }
  out->msg[0] |= (parity32(out->msg) << 7);
  out->state = START;
  if (dir) {
    TIMSK |= (1 << OCIE0B); // Enable klok 0 interrupt compare B
  } else {
    TIMSK |= (1 << OCIE0A); // Enable klok 0 interrupt compare A
  }
}

ISR(TIMER0_COMPA_vect) {
  manch_encode(&out_to_boiler, (1 << TO_BOILER), (1 << OCIE0A));
}

ISR(TIMER0_COMPB_vect) {
  manch_encode(&out_to_therm, (1 << TO_THERM), (1 << OCIE0B));
}


// ============================== ontvangen ==============================


void in_handler(volatile in_t *in, uint8_t in_mask, uint8_t out_mask) {
  uint16_t tc1_value;

  /*
   * Lees de counter uit om periode van de pulse te kunnen
   * bepalen (wordt in manchester decoder pas gedaan. Reset de
   * counter daarna onmiddelijk zodat bij de volgende interrupt
   * de periode zo nauwkeurig mogelijk bepaald kan worden.
   */
  tc1_value = TCNT1;
  TCNT1 = 0; // reset timer 1 counter.

  switch (mode) {
  case INTERCEPT:
  case MONITOR:
    manch_decode(in, tc1_value);
    if (in->state == DONE) {  		// Hele bericht binnen?
      if (in->parity) {			// Als oneven aantal bits gelezen?
	in->state = PARITY_ERROR;	// hebben we een parity error.
      } else {
	in->msg[0] &= 0x7F; 		// Haal parity bit weg.
      }
    }
    if (mode == INTERCEPT) {
      break;
    }
    // fall thru
  case PASSTHRU: 			// Kopieer input naar output
    if (PIND & in_mask) { 		// LET OP: geinverteerde boel!
      PORTD &= ~out_mask;
      PORTB |= (1 << LED3);
    } else {
      PORTD |= out_mask;
      PORTB &= ~(1 << LED3);
    }
    break;
  }
}

/*
 * Data van de thermostaat of boiler vuurt een interrupt. 
 */
ISR(INT0_vect) {
  in_handler(&in_from_boiler, (1 << FROM_BOILER), (1 << TO_THERM));
}

ISR(INT1_vect) {
  in_handler(&in_from_therm, (1 << FROM_THERM), (1 << TO_BOILER));
}

/*
 * Reset / sync de ingangen. Als er een tijd van meer dan 1 ms is
 * verlopen sinds de vorige interrupt, dan is er geen signaal geweest
 * op de ingangen.  Geldt natuurlijk niet als we net een bericht
 * binnen hebben dat nog moet worden verwerkt. Zonder deze sync krijg
 * je de Manchester coding nauwelijks op gang omdat er smurrie op de
 * lijn zit en extra signalen van OpenTherm 3 Powerboel.
 */
ISR(TIMER1_COMPA_vect) {
  if (in_from_therm.state != DONE) {
    in_from_therm.state = WAITING;
  }
  if (in_from_boiler.state != DONE) {
    in_from_boiler.state = WAITING;
  }
}

/*
 * Als er een bericht ontvangen is, wordt het bericht naar de
 * opgegeven variabele gekopieerd en wordt de ingang weer vrijgegeven
 * om het volgende bericht op te pakken. Om aan te geven dat er een
 * nieuw bericht was, wordt de waarde 1 door de functie
 * teruggegeven.. Als er nog geen bericht binnen was, wordt de waarde
 * 0 teruggegeven. Als er een parity error was, wordt de ingang ook
 * weer gereset en wordt de waarde 0, geen nieuw bericht,
 * teruggegeven. Als je iets wilt emt parity errors, hier inhaken. Nog
 * nooit een parity error gezien, dus hoeft van mij niet.
 */
uint8_t receive(volatile in_t *in, uint8_t msg[]) {
  uint8_t rv = 0;
  
  switch (in->state) {
  case DONE:
    for (uint8_t i = 0; i < FRAME_BYTES; i++) {
      msg[i] = in->msg[i];
    }
    rv = 1;
    // fall thru
  case PARITY_ERROR:
    in->state = WAITING;
    break;
  }
  return rv;
}


// ============================== zend msg naar externe host ==============================

/*
 * Stuur een bericht naar de externe host. We sturen berichten
 * in het zelfde formaat als OpenTherm en dus altijd van een vaste
 * lengte. Vaak zijn het gewoon de berichten die we voorbij hebben
 * zien komen tussen thermostaat en ketel, maar het kunnen ook de
 * resultaten van een commando zijn.
 */
void send_msg_to_host(uint8_t msg[]) {
  for (uint8_t i = 0; i < FRAME_BYTES; i++) {
    uputc(msg[i]);
  }
}

// ============================== set mode ==============================

/* 
 * Zet de globale mode, maar zet ook een van de leds om de mode aan te
 * geven en, heel sneaky, zet de watchdog timer op scherp in het geval
 * de nieuwe modus INTERCEPT of MONITOR is. Bij PASSTHRU wordt de WDT
 * juist weer uit gezet. De watchdog timer zorgt ervoor dat de gateway
 * bij problemen met de extrene host toch weer terugkomt in een
 * bekende status en we niet in de kou komen te zitten.
 */
void set_mode(uint8_t m) {
  mode = m;
  switch (m) {
  case PASSTHRU:
    PORTB &= ~(1 << LED1);
    wdt_disable();
    break;
  case MONITOR:
  case INTERCEPT:
    PORTB |= (1 << LED1);
    wdt_enable(WDTO_8S);
    break;
  }
}

// ============================== commands ==============================

/*
 * Een bericht voor de gw: het tweede byte (na msb) bepaalt net als
 * bij OpenTherm het commando. De data waarden zitten in de laatste
 * twee bytes en waardes die terug moeten naar de externe host moeten
 * in deze laatste bytes worden weggeschreven. Een paar minimale
 * commando's
 */
void process_cmd(uint8_t msg[]) {
  switch (msg[1]) {
  case EOS: 		// end of session
    set_mode(PASSTHRU);
    break;
  case DO_MONITOR:	// ga in MONITOR modues
    set_mode(MONITOR);
    break;
  case DO_INTERCEPT:	// ga in INTERCEPT modus
    set_mode(INTERCEPT);
    break;
  case PING:		// Hard nodig zodat de externe partij kan laten weten dat ze er nog zijn
    msg[3] = 1;		// Voorbeeld van data teruggeven
    break;
  case SET_T_MIN:
    t_min.valueh = msg[2];
    t_min.valuel = msg[3];
  case GET_T_MIN:
    msg[2] = t_min.valueh;
    msg[3] = t_min.valuel;
    break;
  case SET_T_MAX:
    t_max.valueh = msg[2];
    t_max.valuel = msg[3];
  case GET_T_MAX:
    msg[2] = t_max.valueh;
    msg[3] = t_max.valuel;
    break;
  case SET_T2_MIN:
    t2_min.valueh = msg[2];
    t2_min.valuel = msg[3];
  case GET_T2_MIN:
    msg[2] = t2_min.valueh;
    msg[3] = t2_min.valuel;
    break;
  case SET_T2_MAX:
    t2_max.valueh = msg[2];
    t2_max.valuel = msg[3];
  case GET_T2_MAX:
    msg[2] = t2_max.valueh;
    msg[3] = t2_max.valuel;
    break;
  default:
    msg[1] = UNKNOWN_DATAID; // Onbekend commando, laat externe dat ook weten
  }
  send_msg_to_host(msg);
}

// ============================== Watchdog ==============================

/*
 * WDT wordt aangezet als gw in MONITOR / INTERCEPT mode is. Als dan
 * de externe host (Rpi / Arduino) een bepaalde tijd niets van zich
 * laat horen, moet de mode terug worden gezet naar PASSTHRU. Op die
 * manier hou je de boel een beetje warm in huis als de externe
 * applicatie crasht.
 */
ISR(WDT_OVERFLOW_vect) {
  set_mode(PASSTHRU);
}

// ============================== Mainloop ==============================

int main(void) {

  uint8_t msg[FRAME_BYTES];
  uint8_t i;
  uint8_t c;

  /* 
   * If a reset was caused by the Watchdog Timer, clear the WDT reset
   * flag, enable the WD Change Bit and disable the WDT.
   */
  if (MCUSR & (1 << WDRF)) {
    MCUSR &= ~(1 << WDRF);
    WDTCR |= ((1 << WDCE) | (1 << WDE));
    WDTCR = 0x00;
  }
  mode = PASSTHRU;
  init();
  sei();

  for (;;) {

    if (mode == PASSTHRU) {
      /*
       * Wacht op een ENQ karakter van bijv. Rpi of Arduino als teken
       * dat deze de controle over de gw wil overnemen. Stuur elke
       * HOST_CONNECT_RETRY ms opnieuw een ENQ als er niets komt. Als
       * de andere kant antwoord met een SYN karakter, schakelen we
       * van de PASSTHRU naar de MONITOR mode waarmee we ook onder
       * controle van de extrene partij zijn gekomen.
       */
      do {
	do {
	  uputc(ENQ);
	  _delay_ms(HOST_CONNECT_RETRY);
	} while (!(ugetc_nb(&c)));
      } while (c != SYN);

      uputc(ACK); 	// Acknowledge
      i = 0;
      set_mode(MONITOR);

    } else {

      /*
       * Geen PASSTHRU mode, dus kijk of er een karakter van Rpi /
       * Arduino is. ugetc_nb is 'non-blocking'. Berichten van de
       * externe host komen, net als OpenTherm bercichten, per vier
       * bytes. Zo kan de externe host gewoon OT berichten de gw
       * insturen die afhankelijk van de msb de goede kant opgaan. De
       * gereserveerde bit combinaties in de msb gebruiken we als
       * teken dat het bericht voor de gw zelf is bedoeld. Als we met
       * regelmaat iets binnenkrijgen van de externe host, resetten we
       * de watchdog timer op tijd om niet terug naar de PASSTHRU mode
       * te gaan.
       */
      if (ugetc_nb(&c)) {		// Nieuw karakter?
	msg[i] = c;			// plaats in buffer
	if (i == FRAME_BYTES - 1) { 	// Bericht binnen?
	  wdt_reset();			// Hond in zijn hok
	  if ((msg[0] & MSGID_MSK) == HOST_TO_GW) { // Bwericht voor gw bedoeld?
	    process_cmd(msg);
	  } else {
	    send(msg);			// Stuur door naar MASTER of SLAVE
	  }
	  i = 0;
	} else {
	  ++ i;
	}
      }

    }  /* if (mode == PASSTHRU)... */
    
    /*
     * Als mode PASSTHRU of MONITOR is, worden de inkomende signalen
     * al meteen doorgestuurd naar de uitgang. Als de mode INTERCEPT
     * is, dan worden de berichten doorgestuurd naar de externe host
     * en die moet maar zien dat er iets wordt doorgestuurd.
     */
    if (mode != PASSTHRU) {
      if (receive(&in_from_therm, msg)) {
	send_msg_to_host(msg);
      }
      if (receive(&in_from_boiler, msg)) {
	send_msg_to_host(msg);
      }
    }
     
  }  /* for... */
}
