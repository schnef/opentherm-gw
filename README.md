# OpenTherm Gateway

Deze gateway maakt het mogelijk om het dataverkeer tussen een
thermostaat en een CV-ketel te monitoren of om te lieden via een
externe applicatie. In monitor mode kan data worden verzameld, maar in
de **INTERCEPT** mode kan de externe applicatie de volledige controle
overnemen.

De gateway is momenteel geschikt voor het OpenTherm 2.2 protocol.

## Hardware

De hardware is nagenoeg een volledige kopie van dit
[project](http://otgw.tclcode.com/index.html). Het grote verschil zit
hem in het gebruik van een [Raspberry pi](http://www.raspberrypi.org/)
of Arduino als host. Dus geen RS232, USB of wat dan ook, maar direct
verbonden.

Door de gatway direct aan te sluiten op de Rpi heb je niet alleen
direct de verbinding voor elkaar, maar je kunt de gateway ook direct
vanuit de Rpi programmeren. De Rpi levert ook de voeding.

Een ander verschil in het ontwerp is de *MCU* die wordt gebruikt. Ik
heb gekozen voor de ATtiny4313, omdat ik daar nou eenmaal beter mee
uit de voeten kan.  De ATtiny4313 is het net ietsjes grotere broertje
van de ATtiny2313 en heeft voldoende flash geheugen om de firmware in
kwijt te kunnen.

### Waarschuwing

Gebruik van de hardware is geheel op eigen risico. Op geen enkele
manier ben ik aansprakelijk voor het gebruik van de hardware.

## Firmware

Het hele project is ooit begonnen om te kunnen experimenteren met
[Manchester
codering](http://en.wikipedia.org/wiki/Manchester_code). Het OpenTherm
protocol maakt gebruik van Manchester codering en het bleek redelijk
eenvoudig om coderen en decoreren als FSM in de ATtiny te
implementeren.

De implementatie is volledig gebaseerd op de relatieve duur van de
data overgangen. Er wordt dus niet gekeken naar signaalniveaus maar de
tijden tusssen overgangen in de data. Polariteit van het signaal is
dan ook niet meer relevant.

De timing van de codering bleek nauwelijks kritisch en grote
afwijkingen in de timing zijn geen probleem. De implementatie kan veel
hogere snelheden aan dan de voor OpenTherm gedefinieerde timing.

De firmware is zeer rudimentair maar is ook zeer betrouwbaar gebleken
in de praktijk. Het is vooral de bedoeling dat een extrne applicatie
de moeilijke dingen doet en de firmware van de gateway zich beperkt
tot het hoog nodige. Er is voldoende ruimte in flash over om extra
functionaliteit in te bouwen en de C-sources spreken voor zich.

In de map *python* is een heel eenvoudig Python programma opgenomen om
de gateway te testen en als voorbeeld voor andere programma's.

### ATtiny4313 programmeren Raspberry Pi

Het lukte mij niet met de standaad avrdude op de Rpi de ATtiny4313 te
programmeren. Kwestie van de niewste avrdude vanaf sources te
installeren. Kan me niet meer heel precies herinneren hoe het ging,
maar het was iets als volgt:

`
sudo apt-get install bison automake autoconf flex gcc
`
`
sudo apt-get install gcc-avr binutils-avr avr-libc
`

Haal [hier](http://download.savannah.gnu.org/releases/avrdude/) de
nieuwe versie 6 sources op.

`
cd avrdude/avrdude
`
`
./bootstrap && ./configure && sudo make install
`

## Links

- [Controlling the central heating system](http://otgw.tclcode.com/index.html);
- [OpenTherm Protocol Specifications 2.2](http://www.domoticaforum.eu/uploaded/Ard%20M/Opentherm%20Protocol%20v2-2.pdf);
- [Atmel Manchester Coding Basics Application Note](http://www.nesweb.ch/downloads/doc9164.pdf);
- [ATtiny2313A/4313 datasheet](http://www.atmel.com/images/doc8246.pdf);
- [8-bit Precision A/D Converter](http://www.atmel.com/Images/doc0953.pdf) en de beschrijving hier van de [Low-cost zero-CPU-load analog-to-digital converter](http://www.keesmoerman.nl/attiny_hw.html) waarmee je bijv. een TMP36 temperatuur sensor aan de gatway kunt koppelen.
- [AVR Libc Home Page](http://www.nongnu.org/avr-libc/).

## Licentie

De software is gepubliceerd onder de MIT licentie. Van het originele
hardware is niet bekend onder welke licentie deze is
gepubliceerd. Voor de hardware geldt net als voor de software dat het
gebruik geheel vooreigen risico is.

The MIT License (MIT)

Copyright (c) 2015 Frans Schneider

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

