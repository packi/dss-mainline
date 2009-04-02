#include <pic16f628.h>

typedef unsigned int word;
word at 0x2007  __CONFIG = _CP_OFF & _WDT_OFF & _BODEN_ON & \
         _XT_OSC & _MCLRE_OFF & \
         _LVP_OFF;

void putch( unsigned char _c ) {
  // wait for TX-Queue to get empty
  while( !TXIF )
  { }

  TXREG = _c;
}

static void initializePorts() {
  CMCON = 0x07; // disable comparators
  PORTA = 0x00;
  PORTB = 0x00;
  
  TRISA = 0x07; // set RA0-2 as outputs
  TRISB = 0x00;

  SPBRG=21; // 56000 baud @ 20MHz
  BRGH=1; // high baud rate
  SYNC=0; // asyncronous mode
  SPEN=1; // enable serial port
  CREN=1; // continous receive
  SREN=0; // disable single receive (don't care in async mode)
  TXIE=0; // disable tx-interrupt
  RCIE=0; // enable tx-interrupt
  TX9=0; // 8-bit communication
  RX9=0; // also for receiving
  TXEN=0; // disable transmitter
  TXEN=1; // and re-enable it to reset
} // initializePorts


void main() {
  unsigned char currentCnt;
  unsigned char vals[3];
  unsigned char tmp;
  initializePorts();

  currentCnt = 0;
  vals[0] = 128;
  vals[1] = 64;
  vals[2] = 30;
  tmp = 0;

  RB1 = 1;
  
  while(1) {
    RB0 = vals[0] < currentCnt;
    RB6 = vals[1] < currentCnt;
    RB7 = vals[2] < currentCnt;
    RB2 = tmp < currentCnt;
    currentCnt++;
    if(currentCnt == 0) {
      tmp++;
    }
  } 

} // main
