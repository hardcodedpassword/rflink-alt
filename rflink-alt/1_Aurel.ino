// This tab contains the code to interface with the Aurel RTX MID 5v
// Datasheet to be found here: http://www.aurelwireless.com/wp-content/uploads/user-manual/650201033G_um.pdf
// Send and receiving pulses is done using interrupt functions for high temporal accuracy
// Pulses are measured and specified in 0.010 ms i.e. units of 10us (see PULSE_RESOLUTION)
// Note that the aurel is not capable of receiving MAX_PULSES < 100us and > 5000us
//
// TODO: need to convert this part to a class.

#define MAX_PULSES        800
#define MIN_PULSES        20
#define MIN_PULSE_TIME    90   // us - note that 100us is the shortest pulse the Aurel can reliably receive
#define MAX_PULSE_TIME    2550  // us - max to fit in a byte
#define PULSE_RESOLUTION  10    // us
#define PULSE_RESOLUTION_ROUND_CORRECTION 5 // us

// Some macros to manipulate the transmit output
#define TX_LOW    (*_TxReg &= ~_TxBit)
#define TX_HIGH   (*_TxReg |=  _TxBit)
#define TX_FLIP   (*_TxReg ^=  _TxBit)
#define TX_VALUE  (*_TxReg &   _TxBit)

// Macro to read the input
#define RX_VALUE    (*RxReg & RxBit)

// variables for direct in & output register access
byte _RxBit;
byte _RxPort;
byte _TxBit;
byte _TxPort;
volatile byte* _RxReg;
volatile byte* _TxReg;

// Pulse buffers:
// A '1' pulse is always followed by a 0and a '0' pulse is followed by a 1 and so on.
// Stored are the duration (in 10th  microseconds or 1/100 millseconds) of each 0 or 1.
// 

// Receiving:
// two buffers for receiving data. Once a message is complete, the receiver ISR will switch to the other buffer.
byte  _pulsebuf[2][MAX_PULSES]; // Pulse is either an 1 or a 0.
byte* _dataRdy = _pulsebuf[0];
byte* _dataRec = _pulsebuf[1];
unsigned int _counter = 0;
unsigned int _pulses = 0;
volatile bool _messageFlag = false;

// Sending:
// buffer with pulses to send
// must be terminated with a zero value!
byte  _sendBuf[MAX_PULSES+3]; // bytes extra for terminating 0, repeats, and delay between repeats.
volatile byte* _sendPtr;

// ISR for receiving pulses
void AurelIntRx( void )
{
  static unsigned long _lastTime = 0;
  const  unsigned long now = micros();  // TODO: use timer value capture on input change for higher accury
  const  unsigned long duration = now - _lastTime;

  // first pulse must be a HIGH and long enough
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
      // switch the buffers:
      byte* tmp = _dataRdy;
      _dataRdy = _dataRec;
      _dataRec = tmp;
      _pulses = _counter;
      _messageFlag = true;
    }

    // reset
    _lastTime = now;
    _counter = 0;
    return;
  }

  // valid pulse -> store
  byte d = ((duration + PULSE_RESOLUTION_ROUND_CORRECTION) / PULSE_RESOLUTION); // and compensate for integer rounding
  _dataRec[ _counter ] = d;
  _lastTime = now;
  _counter++;
}

// returns true if a complete message is available
bool IsMsgAvail( byte **p, unsigned int* count )
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

  pinMode( ENABLE, OUTPUT );
  pinMode( TX_RX, OUTPUT );
  pinMode( DATA_RX, INPUT_PULLUP );
  pinMode( DATA_TX, OUTPUT );

  digitalWrite( ENABLE, LOW );
  digitalWrite( TX_RX, LOW );
  digitalWrite( DATA_RX, LOW );
  digitalWrite( DATA_TX, LOW );
  // and give the Aurel some time to become stable
  delayMicroseconds( 500 );

  // Prepare Timer1 for transmission
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  // set CTC mode
  TCCR1B &= ~(1 << WGM13);
  TCCR1B |=  (1 << WGM12);

  // set to prescaler 8
  // 1 tick equals 8 / 16Mhz = 0.0000005seconds = 0.0005ms = 0.5us
  //
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

void AurelRxToTX(void)
{
  digitalWrite( DATA_RX, LOW );
  digitalWrite(TX_RX, HIGH);
  // delayMicroseconds(500); This delay is handled in the TX function
}

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
    TIMSK1 &= !(1 << OCIE1A); // disable timer interrupt
    return;
  }
  int duration = *_sendPtr;

  // check end of message
  if ( duration == 0 )
  {
    TIMSK1 &= !(1 << OCIE1A); // disable timer interrupt
    _sendPtr = 0;
    TX_LOW; // force low
    return;
  }

  // Flip pulse
  TX_FLIP;
  OCR1A = duration * PULSE_RESOLUTION * 2;  // calculate pulse length
  _sendPtr++;
}

// returns true while transmission is ongoing
bool TxBusy()
{
  return TIMSK1 & (1 << OCIE1A);
}


// Prepare transmit a sequence of pulses.
void TX()
{
  int repeats = _sendBuf[0];
  int delayTime = _sendBuf[1];
  
  AurelRxToTX();

  for ( int r = 0; r < repeats; r++ )
  {
    noInterrupts();
    TCNT1 = 0;
    OCR1A = 1000;  // compare match register to 500us when 1st pulse starts
    _sendPtr = _sendBuf+2; // first two bytes contain repeats and delaytime so skip these
    TX_LOW;
    TIMSK1 |= (1 << OCIE1A); // enable timer 1 interrupts
    interrupts();

    while ( TxBusy() )
    {
      delay(1);
    }

    // no delay needed after last transmit
    if ( r < (repeats - 1) )
    {
      delay( delayTime );
    }
  }
  AurelTxToRX();
}
