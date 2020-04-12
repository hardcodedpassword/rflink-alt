# rflink-alt
Alternative software for the rflink tranceiver (based on **Arduino Mega and the Aurel RTX MID 5V** "NODO shield")

**Major goal**: increased flexibility and open source
The firmware will not support the RFLink Serial protocol. Instead it will be a 'transparent' interface, sending received pulses to the host and the host send pulse buffers to RFLink-als to for transmission. This way the RFLink-alt needs know knowledge of any of the 433 protocols. Instead the Host can parse and assemble message.

When a message is received the RFLink-alt will echo on serial:
r:[number of pulses]:[pulse length],[pulse length],[pulse length],[pulse length],
e.g.: "r:21:20,40,20,40,20,40,20,40,20,40,20,40,20,40,20,40,20,40,40,20,40"
**The PUSLE LENGTHs are in units of 10us!!**

TODOs:
* Serial protocol implementation for transmitting: receiving Pulse buffers from the serial and transmitting these
* Code in the 'Aurel' tab must be refactored into a class
* Receive ISR should use a timer to measure the length of pulses for higher temporal accuray
* Remove the test code in test tab
