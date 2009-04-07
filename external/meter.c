#include <pic16f628.h>

// Fake power-meter
// Transmits fake measurements over RS232 and blinks a LED

typedef unsigned int word;
word at 0x2007  __CONFIG = _CP_OFF & _WDT_OFF & _BODEN_ON & \
         _XT_OSC & _MCLRE_OFF & \
         _LVP_OFF;

static void putch( unsigned char _c ) {
  // wait for TX-Queue to get empty
  while( !TXIF )
  { }

  TXREG = _c;
}

static void sleep();

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


#define LEDPORT RB0

void main() {
  unsigned char currentCnt;
  unsigned char vals[3];
  unsigned char tmp;
  unsigned int meterValue;
  unsigned char iByte;
 
  initializePorts();
  meterValue = 0;
  
  while(true) {
    sleep();
    LEDPORT = !LEDPORT;
    iByte = sizeof(meterValue) - 1;
    while(iByte > 0) {
      putch((meterValue >> iByte) & 0x00FF);
    }
    sleep();
  }
} // main


static void sleep() {
  int counter1;
  int counter2;
  
  counter1 = 0xFFFF;  
  while(counter1 > 0) {
    counter1--;
    counter2 = 0xFFFF;
    while(counter2 > 0) {
      counter2--;
    }
  }
}