# rflink-alt
Alternative software for the rflink tranceiver (based on **Arduino Mega and the Aurel RTX MID 5V** "NODO shield")

**Major goal**: increased flexibility and open source
The firmware will not support the RFLink Serial protocol. Instead it will be a 'transparent' interface, sending received pulses to the host and the host can send pulse buffers to RFLink-alt to be transmitted. 
By this design the RFLink-alt does not have knowledge of the various 433MHz protocols. It will be the responsibility of the host to parse received pulse sequences and construct pulse sequences that can be transmitted.
Normally the host is a PC or raspberry platform that is much more capable of performing such tasks.
This allows also easy capturing and retransmission of sequences and makes copying of remotes easy.

The host interface is realized by the Arduino Mega its usb serial port interface. The commands and responses are human readable.

** RECEIVING: **
When the RFLink-alt has received a valid pulse sequence it will send the pulse sequence to the host.
RfLink-Alt -> Host messages  starting with "r:" are received pulses:
r:[number of pulses]:[pulse length],[pulse length],[pulse length],[pulse length],
e.g.:
"r:21:20,40,20,40,20,40,20,40,20,40,20,40,20,40,20,40,20,40,40,20,40"
All PUSLE LENGTHs are in units of 10us!!

** TRANSMITTING: **
Host -> RfLink-alt  starting with "t:" are pulses to transmit:
t:[number of repeats],[delay between repeats],[pulse length],pulse length],...
example pulses to send:
"t:10,40,35,35,70,70,35,35,70,35,35,70,70,35,70,70,35,70,35,70,70,35,35,70\n"
All PUSLE LENGTHs are in units of 10us !!
Do NOT use zeros !!

TODOs:
***  Code in the 'Aurel' tab must be refactored into a class


Changes:
- decreased MIN_PULSE_TIME from 100 to 90 us.
- implemented interpreter for sending pulses
- removed test code

V0.0.3:  
- Added version number, added code comments, some minor changes
- fixed receive bug
- decreased MIN_PULSE_TIME from 90 to 85 us
