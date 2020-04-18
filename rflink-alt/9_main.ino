
// Prints the content of the pulse buffer
void PrintPulseBuffer( byte* p, int pulses )
{
  int n;
  Serial.print("r:"); Serial.print( pulses ); Serial.print(':');
  for ( n = 0; n < pulses - 1; n++, p++ )
  {
    Serial.print( *p );
    Serial.print(",");
  }
  Serial.println( *p );
}

void setup()
{
  Serial.begin(57600);
  Serial.setTimeout(200);
  AurelSetup();
  // set aurel in receive mode
  AurelPwrdToRx();
  Serial.print( "i:RFLink-alt v" ); Serial.println( RFLINK_ALT_VERSION );
  Serial.print( "i:Pulse timebase is "); Serial.print( PULSE_RESOLUTION ); Serial.println(" microseconds.");
}

void loop()
{
  if ( Serial.available() )
  {
    // temporary test code
    byte input = Serial.read();

    // t:123,123,123,123,123,123 \n or \r or \0 or whatever
    if ( input == 't' )
    {
      // start receiving pulse info from host for transmission

      long n;
      int nofPulses = 0;
      byte* ptr = _sendBuf;

      // parseInt is a bit weird, it will return a 0 on timeout and a 0 on "0"
      // so you shouln'd send any "0" numbers in the pulse sequence
      n = Serial.parseInt();

      while ( (n > 0) && (nofPulses < MAX_PULSES) )
      {
        *ptr = (byte)n;
        ptr++;
        nofPulses++;
        n = Serial.parseInt();
      }
      *ptr = 0; // add terminating 0;
      TransmitPulseSequence();
      Serial.print( "i:Transmitted: " );     Serial.print( nofPulses - 2 ); Serial.println(" pulses" );
      Serial.print( "i:Repeats: " );         Serial.println( _sendBuf[0] );
      Serial.print( "i:Repeat delay: " );    Serial.print( _sendBuf[1] ); Serial.println(" ms" );
    }
  }
  // check if a message has been received
  // and echo it to the host.
  byte* ptr;
  unsigned int count;
  if ( IsMsgAvail( &ptr, &count ) )
  {
    PrintPulseBuffer( ptr, count );
  }
}
