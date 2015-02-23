#ifndef PROTOCOL_H_
#define PROTOCOL_H_

// ASCII characters used during handshake
#define ENQ 		0x05
#define SYN 		0x16
#define ACK 		0x06

// OpernTherm msg-id = first byte
#define MSGID_MSK 	0b01110000
#define MSTR_TO_SLV_BIT	6
#define READ_DATA 	0x00
#define WRITE_DATA 	0x10
#define INVALID_DATA 	0x20
#define HOST_TO_GW 	0x30 // = reserved
#define READ_ACL 	0x40
#define WRITE_ACK 	0x50
#define DATA_INVALID 	0x60
#define UNKNOWN_DATAID 	0x70

// OpernTherm data-id = second bye
#define GET_SET_FLG	0x80

// Dit zijn commando's tussen gw en externe controller
#define EOS 		0x01
#define PING		0x02
#define RESTART		0x03
#define DO_MONITOR	0x04
#define DO_INTERCEPT	0x05
#define GET_TEMPR	0x06
#define GET_T_DIV	0x07
#define SET_T_DIV	0x87
#define GET_T		0x08
#define SET_T		0x88
#define GET_T2		0x09
#define SET_T2		0x89
#define GET_T_MIN	0x0A
#define SET_T_MIN	0x8A
#define GET_T_MAX	0x0B
#define SET_T_MAX	0x8B
#define GET_T2_MIN	0x0C
#define SET_T2_MIN	0x8C
#define GET_T2_MAX	0x0D
#define SET_T2_MAX	0x8D
#define GET_LED		0x0E
#define SET_LED		0x8E
#define GET_BAUD	0x0F
#define SET_BAUD	0x8F
#define GET_PAR_ERR_CNT	0x10
#define SET_PAR_ERR_CNT	0x90
#define GET_FRM_ERR_CNT	0x11
#define SET_FRM_ERR_CNT	0x91
#define GET_SYN_ERR_CNT	0x12
#define SET_SYN_ERR_CNT	0x92
#define GET_TEST	0x13
#define SET_TEST	0x93
#define DO_TEST		0xFF

#endif /* PROTOCOL_H_ */
