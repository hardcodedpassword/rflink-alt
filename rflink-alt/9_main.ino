
// Prints the content of the pulse buffer
void PrintPulseBuffer( byte* p, int pulses )
{
  int n;
  Serial.print("r:"); Serial.print( pulses ); Serial.print(':');
  for ( n = 0; n < pulses; n++, p++ )
  {
    Serial.print( *p );
    Serial.print(",");
  }
 
  Serial.println();
}

void setup() {
  Serial.begin(57600);  
  AurelSetup();
  // set aurel in receive mode
  AurelPwrdToRx();
  Serial.println( "RFLink-alt" );
}

void loop() {
  // todo: remove test code below and replace by interpreter 
  if( Serial.available() ) 
  {
    // temporary test code
    byte input = Serial.read();
    if ( input == 't' )
    {
      transmit1();
    }
    if ( input == 'r' )
    {
      transmit2();
    }    
  }

  // check if a message has been received 
  // and send it to the host.
  byte* ptr;
  unsigned int count;
  if ( IsMsgAvail( &ptr, &count ) )
  {
    PrintPulseBuffer( ptr, count );
  }

}
