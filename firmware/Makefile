#    This is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this.  If not, see <http://www.gnu.org/licenses/>.
#################################################################################

TARGET=main

MCU=attiny4313
FREWQ=11059200UL

# Debug
#DEBUG	= -gstabs

# C flags

CC	= avr-gcc
#CFLAGS	= $(DEBUG) -O3                  -Wall -std=gnu99 -mmcu=$(MCU) -DF_CPU=$(FREWQ) $(INCLUDE)
CFLAGS	= $(DEBUG) -O2 -mcall-prologues -Wall -std=gnu99 -mmcu=$(MCU) -DF_CPU=$(FREWQ) $(INCLUDE)

LD	= avr-gcc
#LDFLAGS2=-Wl,-uvfprintf -lprintf_flt
LDFLAGS = -mmcu=$(MCU) $(DEBUG) $(LIBLOC) $(LDFLAGS2)
#LIBS    = -ldross -lm

#################################################################################

SRC	=	$(TARGET).c serial.c manchester.c

OBJ	=	$(SRC:.c=.o)

LST	=	$(SRC:.c=.lst)

all:	$(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	@echo [hex] $<
	@avr-objdump -h -S $< > $(TARGET).lst
	@avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex

$(TARGET).elf:	$(OBJ) 
	@echo [Link] $<
	@$(LD) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)
	@avr-size $(TARGET).elf

# Generate .lst file rule

%.lst : %.o
	@echo [lst] $<
	@avr-objdump -h -S $< > $@

.c.o:
	@echo [CC] $<
	@$(CC) -c $(CFLAGS) $< -o $@

.PHONEY:	clean
clean:
	rm -f *.o *.elf *.hex *.lst Makefile.bak *~

.PHONEY:	depend
depend:
	makedepend -Y $(SRC)

.PHONEY:	upload
upload:	$(TARGET).hex
	@echo -n "Program FLASH ... "
	@avrdude -q -p $(MCU) -c gpio -U flash:w:$(TARGET).hex:i
	@echo Done.

# See http://www.engbedded.com/fusecalc/
fuse:
	@echo -n "Program FUSES ... "
	@avrdude -q -p $(MCU) -c gpio -U lfuse:w:0x64:m -U lfuse:w:0xce:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
	@echo Done.

rfuse:
	@echo -n "Read FUSES ... "
	@avrdude -q -p $(MCU) -c gpio -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
	@echo Done.

dumpflash:
	@echo -n "Dump FLASH ... "
	@avrdude -q -p $(MCU) -c gpio -U flash:r:flash.bin:r 
	@echo Done.

# DO NOT DELETE
