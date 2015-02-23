#!/usr/bin/env python

import serial
import sys
import argparse
from datetime import datetime
from time import sleep, time

ENQ = 0x05
SYN = 0x16
ACK = 0x06

MSGID_MSK = 0b01110000
MSTR_TO_SLV_BIT = 6
READ_DATA = 0x00
WRITE_DATA = 0x10
INVALID_DATA = 0x20
HOST_TO_GW = 0x30
READ_ACL = 0x40
WRITE_ACK = 0x50
DATA_INVALID = 0x60
UNKNOWN_DATAID = 0x70

EOS = 0x01
PING = 0x02
RESTART = 0x03
DO_MONITOR = 0x04
DO_INTERCEPT = 0x05
GET_TEMPR = 0x06
GET_T_DIV = 0x07
SET_T_DIV = 0x87
GET_T = 0x08
SET_T = 0x88
GET_T2 = 0x09
SET_T2 = 0x89
GET_T_MIN = 0x0A
SET_T_MIN = 0x8A
GET_T_MAX = 0x0B
SET_T_MAX = 0x8B
GET_T2_MIN = 0x0C
SET_T2_MIN = 0x8C
GET_T2_MAX = 0x0D
SET_T2_MAX = 0x8D
GET_LED = 0x0E
SET_LED = 0x8E
GET_BAUD = 0x0F
SET_BAUD = 0x8F
GET_PAR_ERR_CNT = 0x10
SET_PAR_ERR_CNT = 0x90
GET_FRM_ERR_CNT = 0x11
SET_FRM_ERR_CNT = 0x91
GET_ERR_CNT = 0x10
SET_ERR_CNT = 0x90
GET_SYN_ERR_CNT = 0x12
SET_SYN_ERR_CNT = 0x92
GET_TEST = 0x13
SET_TEST = 0x93
DO_TEST = 0xFF

MASTER_TO_SLAVE = 0
SLAVE_TO_MASTER = 1
direction = {
    MASTER_TO_SLAVE: "M -> S",
    SLAVE_TO_MASTER: "M <- S"
}

READ_DATA = 0
WRITE_DATA = 1
INVALID_DATA = 2
RESERVED = 3
READ_ACK = 4
WRITE_ACK = 5
DATA_INVALID = 6
UNKNOWN_DATAID = 7

msg_type = {
    READ_DATA: "READ_DATA",
    WRITE_DATA: "WRITE_DATA",
    INVALID_DATA: "INVALID_DATA",
    RESERVED: "RESERVED",
    READ_ACK: "READ_ACK",
    WRITE_ACK: "WRITE_ACK",
    DATA_INVALID: "DATA_INVALID",
    UNKNOWN_DATAID: "UNKNOWN_DATAID"
}

data_id = {
    0: ("Status", "Master and Slave Status flags. "),
    1: ("TSet", "Control setpoint ie CH water temperature setpoint (C) "),
    2: ("M-Config / M-MemberIDcode", "Master Configuration Flags / Master MemberID Code "),
    3: ("S-Config / S-MemberIDcode", "Slave Configuration Flags / Slave MemberID Code "),
    4: ("Command", "Remote Command "),
    5: ("ASF-flags / OEM-fault-code", "Application-specific fault flags and OEM fault code "),
    6: ("RBP-flags", "Remote boiler parameter transfer-enable & read/write flags "),
    7: ("Cooling-control", "Cooling control signal (%) "),
    8: (" TsetCH2", "Control setpoint for 2e CH circuit (C) "),
    9: ("TrOverride", "Remote override room setpoint "),
    10: ("TSP", "Number of Transparent-Slave-Parameters supported by slave "),
    11: ("TSP-index / TSP-value", "Index number / Value of referred-to transparent slave parameter. "),
    12: ("FHB-size", "Size of Fault-History-Buffer supported by slave "),
    13: ("FHB-index / FHB-value", "Index number / Value of referred-to fault-history buffer entry. "),
    14: ("Max-rel-mod-level-setting", "Maximum relative modulation level setting (%) "),
    15: ("Max-Capacity / Min-Mod-Level", "Maximum boiler capacity (kW) / Minimum boiler modulation level(%) "),
    16: ("TrSet", "Room Setpoint (C) "),
    17: ("Rel.-mod-level", "Relative Modulation Level (%) "),
    18: ("CH-pressure", "Water pressure in CH circuit (bar) "),
    19: ("DHW-flow-rate", "Water flow rate in DHW circuit. (litres/minute) "),
    20: ("Day-Time", "Day of Week and Time of Day "),
    21: ("Date", "Calendar date "),
    22: ("Year", "Calendar year "),
    23: ("TrSetCH2", "Room Setpoint for 2nd CH circuit (C) "),
    24: ("Tr", "Room temperature (C) "),
    25: ("Tboiler", "Boiler flow water temperature (C) "),
    26: ("Tdhw", "DHW temperature (C) "),
    27: ("Toutside", "Outside temperature (C) "),
    28: ("Tret", "Return water temperature (C) "),
    29: ("Tstorage", "Solar storage temperature (C) "),
    30: ("Tcollector", "Solar collector temperature (C) "),
    31: ("TflowCH2", "Flow water temperature CH2 circuit (C) "),
    32: ("Tdhw2", "Domestic hot water temperature 2 (C) "),
    33: ("Texhaust", "Boiler exhaust temperature (C) "),
    48: ("TdhwSet-UB / TdhwSet-LB", "DHW setpoint upper & lower bounds for adjustment (C) "),
    49: ("MaxTSet-UB / MaxTSet-LB", "Max CH water setpoint upper & lower bounds for adjustment (C) "),
    50: ("Hcratio-UB / Hcratio-LB", "OTC heat curve ratio upper & lower bounds for adjustment "),
    56: ("TdhwSet", "DHW setpoint (C) (Remote parameter 1) "),
    57: ("MaxTSet", "Max CH water setpoint (C) (Remote parameters 2) "),
    58: ("Hcratio", "OTC heat curve ratio (C) (Remote parameter 3) "),
    100: ("Remote override function", "Function of manual and program changes in master and remote room setpoint. "),
    115: ("OEM diagnostic code", "OEM-specific diagnostic/service code "),
    116: ("Burner starts", "Number of starts burner "),
    117: ("CH pump starts", "Number of starts CH pump "),
    118: ("DHW pump/valve starts", "Number of starts DHW pump/valve "),
    119: ("DHW burner starts", "Number of starts burner during DHW mode "),
    120: ("Burner operation hours", "Number of hours that burner is in operation (i.e. flame on) "),
    121: ("CH pump operation hours", "Number of hours that CH pump has been running "),
    122: ("DHW pump/valve operation hours", "Number of hours that DHW pump has been running or DHW valve has been opened "),
    123: ("DHW burner operation hours", "Number of hours that burner is in operation during DHW mode "),
    124: ("OpenTherm version Master", "The implemented version of the OpenTherm Protocol Specification in the master. "),
    125: ("OpenTherm version Slave", "The implemented version of the OpenTherm Protocol Specification in the slave. "),
    126: ("Master-version", "Master product version number and type "),
    127: ("Slave-version", "Slave product version number and type "),
    128: ("SmartPower", "Smart power level change.")
 }

session = None

def get_bit(int_type, offset):
    """testBit() returns a nonzero result, 2**offset, if the bit at
    'offset' is one."""
    mask = 1 << offset
    return(int_type & mask)

def set_bit(int_type, offset):
    """setBit() returns an integer with the bit at 'offset' set to
    1."""
    mask = 1 << offset
    return(int_type | mask)

def clear_bit(int_type, offset):
    """clearBit() returns an integer with the bit at 'offset'
    cleared."""
    mask = ~(1 << offset)
    return(int_type & mask)

def toggle_bit(int_type, offset):
    """toggleBit() returns an integer with the bit at 'offset'
    inverted, 0 -> 1 and 1 -> 0."""
    mask = 1 << offset
    return(int_type ^ mask)

def bin(s):
    """Integer to Bin String"""
    return str(s) if s <= 1 else bin(s >> 1) + str(s & 1)

def process_msg_type(b):
    if b & 0xf:
        print "msg[0] =", b
        #raise ProtocolException("Spare bits are not 0.")
    if get_bit(b, 6):
        msg_type = SLAVE_TO_MASTER
    else:
        msg_type = MASTER_TO_SLAVE
    return (msg_type, ((b & 0x70) >> 4))

def repr_msg(msg):
    d, v = process_msg_type(msg[0])
#    return str(msg).encode("hex")
#    print "%s\t%s\t%s" % (d, v, str(msg).encode("hex"))
    return "%s\t%s" % (direction[d], msg_type[v])

def repr_data_id(msg):
    try:
        mnem, descr = data_id[msg[1]]
    except:
        mnem, descr = "<unknown>", "<unknown>"
    return "%s" % mnem

class GWIOException(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class UnknownDataID(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class ProtocolException(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class Session():
    def __init__(self, ser=None, mode=None):
        self.__serial = ser
        self.mode = mode
        self.__status = False

    def init(self):
        self.__serial.write(chr(SYN))
        c = ord(self.__serial.read(1))
        if (c == ACK):
            self.__status = True
        return self.__status

    def read(self):
        msg_in = bytearray([0, 0, 0, 0])
        if self.__serial.readinto(msg_in) != 4:
            raise GWIOException("Insufficient number of bytes read.")
        return msg_in

    def read_test(self):
        return self.__serial.read(1024)

    def _host_to_gw(self, cmd, msb=0, lsb=0):
        msg_out = bytearray([HOST_TO_GW, cmd, msb, lsb])
        msg_in = bytearray([0, 0, 0, 0])
        rv = ()
        self.__serial.write(msg_out)
        self.__serial.flush()
        if self.__serial.readinto(msg_in) == 4:
            return (msg_in[2], msg_in[3])
        else:
            raise GWIOException("Insufficient number of bytes read.")
 
    def terminate(self):
        if self.__status:
            self._host_to_gw(EOS)
            print "Clean termination."
            self.__status = False
        return self.__status

    def ping(self):
        _, lsb = self._host_to_gw(PING)
        return lsb

    def do_monitor(self):
        self._host_to_gw(DO_MONITOR)
        return True

    def do_intercept(self):
        self._host_to_gw(DO_INTERCEPT)
        return True
    
    def _get_value(self, cmd):
        msb, lsb = self._host_to_gw(cmd)
        return (msb << 8) + lsb

    def _set_value(self, cmd, v):
        msb = (v >> 8) & 0xFF
        lsb = v & 0xFF
        msb, lsb = self._host_to_gw(cmd, msb, lsb)
        return (msb << 8) + lsb

    def get_t_min(self):
        return self._get_value(GET_T_MIN)

    def set_t_min(self, v):
        return self._set_value(SET_T_MIN, v)

    def get_t_max(self):
        return self._get_value(GET_T_MAX)

    def set_t_max(self, v):
        return self._set_value(SET_T_MAX, v)

    def get_t2_min(self):
        return self._get_value(GET_T2_MIN)

    def set_t2_min(self, v):
        return self._set_value(SET_T2_MIN, v)

    def get_t2_max(self):
        return self._get_value(GET_T2_MAX)

    def set_t2_max(self, v):
        return self._set_value(SET_T2_MAX, v)


def session_handler(session):
    t_min = 500
    t_max = 900
    t2_min = 1000
    t2_max = 1800

    session.set_t_min(t_min)
    session.set_t_max(t_max)
    session.set_t2_min(t2_min)
    session.set_t2_max(t2_max)

    old_time = time()
    mode = session.mode
    if mode == DO_MONITOR: 
        session.do_monitor()
    elif mode == DO_INTERCEPT:
        session.do_intercept()
    else:
        exit(1)

    while True:
        try:
            msg = session.read()
            print "%s\t%s\t%s" % (repr_msg(msg), repr_data_id(msg), str(msg[2:]).encode("hex"))
            if time() - old_time >= 6.0:
                session.ping()  # keep alive ping
                old_time = time()
        except GWIOException, e:
            print " * "
        sys.stdout.flush()

def main(ser, mode):
    global session
    while True:
        c = ord(ser.read(1))
        print "main: c = %i, ord(ENQ) = %i" % (c, ENQ)
        if c == ENQ:
            print "initialting session."
            session = Session(ser, mode)
            if session.init():
                print "session initiated."
                session_handler(session)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='OpenTherm host..')
    parser.add_argument("mode", help="Mode the gateway should use.")
    args = parser.parse_args()
    mode = args.mode
    
    if mode == "monitor":
        nmode = DO_MONITOR
    elif mode == "intercept":
        nmode = DO_INTERCEPT
    else:
        raise ValueError("Wrong mode.")

    ser = serial.Serial("/dev/ttyAMA0", 115200, timeout=10)
    try:
        main(ser, nmode)
    except KeyboardInterrupt:
        pass
    finally:
        if isinstance(session, Session):
            session.terminate()
    ser.close()
