# rflink-alt
Alternative software for the rflink tranceiver (based on **Arduino Mega and the Aurel RTX MID 5V** "NODO shield")

**Major goal**: increased flexibility and open source
The firmware will not support the RFLink Serial protocol. Instead it will be a 'transparent' interface, sending received pulses to the host and the host send pulse buffers to RFLink-als to for transmission. This way the RFLink-alt needs know knowledge of any of the 433 protocols. Instead the Host can parse and assemble message.

** RECEIVING: **
RfLink-Alt -> Host  starting with "r:" are received pulses
r:[number of pulses]:[pulse length],[pulse length],[pulse length],[pulse length],
e.g.:
"r:21:20,40,20,40,20,40,20,40,20,40,20,40,20,40,20,40,20,40,40,20,40"
All PUSLE LENGTHs are in units of 10us!!

** TRANSMITTING: **
Host -> RfLink-alt  starting with "t:" are pulses to transmit:
t:[number of repeats],[delay between repeats],[pulse length],pulse length],...
example pulses to send:
"t 10 40 35, 35,  70,  70,  35,  70,  35,  70,  35,  35,  70,  70,  35,  35,  70,  70,  35,  70,  35,  35,  70,  70,  35,  35,  70,  35,  70,  35,  70,  70,  35,  70,  35,  70,  35,  35,  70"
All PUSLE LENGTHs are in units of 10us !!
Do NOT use zeros !!

TODOs:
* Code in the 'Aurel' tab must be refactored into a class
* Receive ISR should use a timer to measure the length of pulses for higher temporal accuray


