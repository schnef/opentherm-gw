#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// modes waarin de gateway kan staan.
#define PASSTHRU        0
#define MONITOR       	1
#define INTERCEPT       2
#define TEST            0xFF

// OpenTherm message frame
#define FRAME_BYTES     4
#define FRAME_BITS      FRAME_BYTES * 8

// logic values: a one is 0xFF and not 1!
#define ZERO            0x00
#define ONE             0xFF

// State machine statussen bij zenden van berichten
#define IDLE            0
#define START           1
#define SEND_START_BIT  2
#define SEND_MSG        3
#define SEND_STOP_BIT   4

// State machine statussen bij ontvangen van berichten
#define WAITING         0
#define RCV_START_BIT   1
#define RCV_MSG         2
#define RCV_MSG_2       3
#define RCV_STOP_BIT    4
#define RCV_STOP_BIT_2  5

// Volgende statussen gelden voor (bijna) alle state machines.
#define STORE_BIT       0x3F
#define LVL_SWITCH	0x7E
#define DONE            0x7F
#define SYNC_ERROR	0xFD
#define PARITY_ERROR 	0xFE
#define ERROR           0xFF

// Hardware gerelateerd
#define LED             PD6
#define LED3		PB4
#define LED2		PB3
#define LED1		PB2

#define FROM_BOILER     PD2
#define FROM_BOILER_INT INT0
#define TO_BOILER       PD4

#define FROM_THERM      PD3
#define FROM_THERM_INT  INT1
#define TO_THERM        PD5

// 8 bit timer bij clock i/o / 64
#define T_1MS_8BIT	86U // ca 500us

// 16 bit timer bij clock i/o / 8
#define T               691U  // ca 500us
#define T_MIN           500U
#define T_MAX           900U
#define T2              1382U
#define T2_MIN          1100U
#define T2_MAX          1800U
#define T_1MS		1382U	// 1 ms

// Communicatie UART en software buffers
#define BAUD            115200UL // Baudrate
#define BUF_SIZE        8        // Send / receive buffer:!!! MOET DEELBAAR ZIJN DOOR 2 !!!
#define HOST_CONNECT_RETRY 100	 // Probeer elke N ms contact te krijgen met externe partij

// MASTER -> SLAVE dan is in msb bit 6 gelijk aan 0, voor SLAVE -> MASTER = 1
#define OT_DIR_MASK	6 


#endif /* CONSTANTS_H_ */
