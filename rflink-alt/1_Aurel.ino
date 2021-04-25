// This tab contains the code to interface with the Aurel RTX MID 5v
// Datasheet to be found here: http://www.aurelwireless.com/wp-content/uploads/user-manual/650201033G_um.pdf
// Send and receiving pulses is done using interrupt functions for high temporal accuracy
// Pulses are measured and specified in 0.010 ms i.e. units of 10us (see PULSE_RESOLUTION)
// Note that the aurel is not capable of receiving MAX_PULSES < 100us and > 5000us
//

#define MAX_PULSES        800   // max nr of pulses for RX and TX
#define MIN_PULSES        15    // received sequences of pulses with less than MIN_PULSES will be discarded
#define MIN_PULSE_TIME    85    // us - datasheet specifies that 100us is the shortest pulse the Aurel can reliably receive
                                // lowered to 85us to catch short signals else discarded
#define MAX_PULSE_TIME    6000  // us - maximum allowed pulse time.
#define PULSE_RESOLUTION  10    // us
#define PULSE_RESOLUTION_ROUND_CORRECTION 5 // us - used to compensate for integer rounding

// Some macros to manipulate the transmit output
#define TX_LOW    (*_TxReg &= ~_TxBit)
#define TX_HIGH   (*_TxReg |=  _TxBit)
#define TX_FLIP   (*_TxReg ^=  _TxBit)
#define TX_VALUE  (*_TxReg &   _TxBit)

#define LED_ON    (*_LEDReg |=  _LEDBit)
#define LED_OFF   (*_LEDReg &= ~_LEDBit)
#define LED_FLIP  (*_LEDReg ^=  _LEDBit)

// Macro to read the input
#define RX_VALUE    (*RxReg & RxBit)

// variables for direct in & output register access
// this is safer and faster in an ISR then DigitalRead and DigitalWrite
byte _RxBit;
byte _RxPort;
byte _TxBit;
byte _TxPort;
byte _LEDBit;
byte _LEDPort;
volatile byte* _RxReg;
volatile byte* _TxReg;
volatile byte* _LEDReg;

// Pulse buffers:
// A '1' pulse is always followed by a 0and a '0' pulse is followed by a 1 and so on.
// Stored are the duration (in 10th  microseconds or 1/100 millseconds) of each 0 or 1.
// 

// Receiving:
// two buffers for receiving data. Once a message is complete, 
// the receiver ISR will switch to the other buffer and the main loop can process the received message.
// 
short  _pulsebuf[2][MAX_PULSES]; // Pulse is either an 1 or a 0.
short* _dataRdy = _pulsebuf[0];  //  _dataRdy is the pointer used by the main loop
short* _dataRec = _pulsebuf[1];  // _dataRec is the pointer used by the ISR
unsigned int _counter = 0;      // number of pulses in buffer pointed to by _dataRdy
unsigned int _pulses = 0;       // pulse counter used by the ISR
volatile bool _messageFlag = false; // flag to signal message received

// Sending:
// buffer with pulses to send. Must be terminated with a zero value!
// _sendBuf[0] = number of times the pulse sequence needs to be repeated
// _sendBuf[1] = delay between repeated pulse sequences
// _sendBuf[2] = first HIGH pulse duration
// _sendBuf[3] = subsequent LOW pulse duration
// _sendBuf[4] = subsequent HIGH pulse duration
// _sendBuf[5] = subsequent LOW pulse duration
// _sendBuf[6] = subsequent HIGH pulse duration
// ... and so on
// _sendBuf[n] = 0 terminates the sequence.
short _sendBuf[MAX_PULSES+3]; // bytes extra for terminating 0, repeats, and delay between repeats.
volatile short* _sendPtr; // used by timer ISR

// ISR on DATA_RX for receiving pulses
// Cannot use timer value capture on input change cause the RX is wired to pd4 on the 2560.
// PD4 does not have time value capture function 
// See datasheet atmel:https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf
// so we need to use the ugly micros() function.
void AurelIntRx( void )
{
  static unsigned long _lastTime = 0;
  const  unsigned long now = micros(); // can be safely used from within an ISR
  const  unsigned long duration = now - _lastTime;

  // first pulse should be a HIGH and long enough
  if ( duration < MIN_PULSE_TIME )
  {
    _lastTime = now;
    _counter = 0;
    return;
  }

  if ( (duration > MAX_PULSE_TIME) || (_counter >= MAX_PULSES) )
  {
    // timeout -> could signal end of message
    if ( _counter > MIN_PULSES )
    {
      // there is some data, signal that it is available and switch buffers
      short* tmp = _dataRdy;
      _dataRdy = _dataRec;
      _dataRec = tmp;
      _pulses = _counter;
      _messageFlag = true;
      LED_FLIP;
    }

    // reset
    _lastTime = now;
    _counter = 0;
    return;
  }

  // valid pulse, lets store it
  short d = ((duration + PULSE_RESOLUTION_ROUND_CORRECTION) / PULSE_RESOLUTION); // and compensate for integer rounding
  _dataRec[ _counter ] = d;
  _lastTime = now;
  _counter++;
}


// returns true if a complete message is available
// the *p argument will be set to the buffer containing the pulses
bool IsMsgAvail( short **p, unsigned int* count )
{
  bool msgAvail;
  noInterrupts();
  *count = _pulses;
  *p = _dataRdy;
  msgAvail = _messageFlag; // test...
  _messageFlag = false;    // and reset!
  interrupts();

  return msgAvail;
}


// This function sets up the interace with the Aurel
void AurelSetup( void )
{
  _RxBit = digitalPinToBitMask(DATA_RX);
  _RxPort = digitalPinToPort(DATA_RX);
  _RxReg = portInputRegister(_RxPort);

  _TxBit = digitalPinToBitMask(DATA_TX);
  _TxPort = digitalPinToPort(DATA_TX);
  _TxReg =  portOutputRegister(_TxPort);

  _LEDBit = digitalPinToBitMask(LED);
  _LEDPort = digitalPinToPort(LED);
  _LEDReg =  portOutputRegister(_LEDPort);  

  pinMode( ENABLE, OUTPUT );
  pinMode( TX_RX, OUTPUT );
  pinMode( DATA_RX, INPUT_PULLUP );
  pinMode( DATA_TX, OUTPUT );
  pinMode( LED, OUTPUT);
  
  digitalWrite( ENABLE, LOW );
  digitalWrite( TX_RX, LOW );
  digitalWrite( DATA_RX, LOW );
  digitalWrite( DATA_TX, LOW );
  LED_ON;
  // and give the Aurel some time to become stable
  delayMicroseconds( 500 );
  LED_OFF;

  // Prepare Timer1 for transmissions
  // but do not activate it yet
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  // set CTC mode
  TCCR1B &= ~(1 << WGM13);
  TCCR1B |=  (1 << WGM12);

  // set to prescaler 8
  // 1 tick equals 8 / 16Mhz = 0.0000005seconds = 0.0005ms = 0.5us
  TCCR1B &= ~(1 << CS12); // 0
  TCCR1B |=  (1 << CS11); // 1
  TCCR1B &= ~(1 << CS10); // 0
  interrupts();

  attachInterrupt( digitalPinToInterrupt(DATA_RX), AurelIntRx, CHANGE );
}


// Set Aurel to RX mode from PWRDN mode
// Preconditions:
//   ENABLE=LOW
//   TX_RX = LOW
void AurelPwrdToRx( void )
{
  digitalWrite(ENABLE, HIGH);
  delayMicroseconds(20);
  digitalWrite(TX_RX, HIGH);
  delayMicroseconds(200);
  digitalWrite(TX_RX, LOW);
  delayMicroseconds(40);
  digitalWrite(ENABLE, LOW);
  delayMicroseconds(20);
  digitalWrite(ENABLE, HIGH);
  delayMicroseconds(200);
}

// Set Aurel from RX to TX mode
// Preconditions:
//   ENABLE = HIGH
//   TX_RX = LOW
void AurelRxToTX(void)
{
  digitalWrite( DATA_RX, LOW );
  digitalWrite( TX_RX, HIGH );
  // a 500 us wait is required for beginnning transmission as specified in the Aurel datasheet 
  // delayMicroseconds(500); This delay is handled in the TX function
}


// Set Aurel from TX to RX mode
// Preconditions:
//   ENABLE = HIGH
//   TX_RX = HIGH
void AurelTxToRX(void)
{
  digitalWrite(TX_RX, LOW);
  delayMicroseconds(40);
  digitalWrite(ENABLE, LOW);
  delayMicroseconds(20);
  digitalWrite(ENABLE, HIGH);
  digitalWrite( DATA_TX, LOW );
  delayMicroseconds(500);
}


// Timer1 interrupt routine used for sending with high time accuracy
ISR( TIMER1_COMPA_vect)
{
  // prevent errors: check on null pointer
  if ( _sendPtr == 0 )
  {
    TIMSK1 &= ~(1 << OCIE1A); // disable timer interrupt
    TX_LOW; // force low
    return;
  }
  int duration = *_sendPtr;

  // check end of message
  if ( duration == 0 )
  {
    TIMSK1 &= ~(1 << OCIE1A); // disable timer interrupt
    TX_LOW; // force low
    _sendPtr = 0;
    return;
  }

  // invert pulse, from high->low or low->high
  TX_FLIP;
  // program next timer interrupt to be the length of this pulse
  OCR1A = duration * PULSE_RESOLUTION * 2;  // pulse length
  _sendPtr++;
}


// returns true while transmission is ongoing
bool TxBusy()
{
  bool busy;
  noInterrupts();
  busy = TIMSK1 & (1 << OCIE1A);
  interrupts();
  return busy;
}


// Prepare transmision of a sequence of pulses.
// Pulses are stored in _sendBuf and must be terminated with a zero value!
// The first two bytes in this buffer have a special meaning
//  _sendBuf[0] = number of times the sequence needs to be repeated [1..2^16]
//  _sendBuf[1] = delay time between repeats (milliseconds) [0..2^16]
// This is a blocking call.
void TransmitPulseSequence()
{
  int repeats = _sendBuf[0];
  int delayTime = _sendBuf[1];
  
  AurelRxToTX();

  for ( int r = 0; r < repeats; r++ )
  {
    noInterrupts();
    TCNT1 = 0;
    OCR1A = 1000;  // Compare match register to 500us when 1st pulse starts, this gives the Aurel time to switch to TX mode
    _sendPtr = _sendBuf+2; // first two bytes contain repeats and delaytime so skip these, first pulse starts at _sendBuf[2]
    TX_LOW;
    TIMSK1 |= (1 << OCIE1A); // enable timer 1 interrupts
    interrupts();

    while ( TxBusy() )
    {
      delay(1);
    }

    // Note: no delay needed after last transmit
    if ( r < (repeats - 1) )
    {
      delay( delayTime ); // milliseconds
    }
  }
  
  AurelTxToRX();
}
