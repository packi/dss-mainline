#include <pic16f628.h>

// Fake power-meter
// Transmits fake measurements over RS232 and blinks a LED

typedef unsigned int word;
word at 0x2007  __CONFIG = _CP_OFF & _WDT_OFF & _BODEN_ON & \
         /*_XT_OSC */ _INTRC_OSC_NOCLKOUT & _MCLRE_OFF & \
         _LVP_OFF;

static void putch( unsigned char _c ) {
  // wait for TX-Queue to get empty
  while( !TRMT )
  { }

  TXREG = _c;
}

static void sleep();

static void initializePorts() {
  CMCON = 0x07; // disable comparators
  PORTA = 0x00;
  PORTB = 0x05; // set tx & rb0 to 1
  
  TRISA = 0xFF;
  TRISB = 0xF2;

  //SPEN = 1;
  //CREN = 1;
  SPBRG=0x19; // 9600 baud @ 20MHz
  //BRGH = 1;
  //TXSTA = TXSTA | 0x20;

  TXSTA = 0x24;
  RCSTA = 0x90;
  /*
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
*/
} // initializePorts


#define LEDPORT RB0

void main() {
  unsigned int tmp;
  unsigned int meterValue;
  unsigned char out;
  unsigned char count;
  unsigned char i;
  unsigned char buf[7];


 
  initializePorts();
  meterValue = 0;
  out = 0xFF;
  count = 0;
  while(1) {
    sleep();
    LEDPORT = out;
    //putch('A');
    //putch('B');
    out = !out;
    if(!out) {
      putch('S');
      tmp = meterValue;
      i = 0;
      do {
        //buf[i] = '0' + tmp % 10;
        //i++;
	putch('0' + tmp % 10);	
      } while( (tmp /= 10) != 0);
      //while(i > 0) {
       // putch(buf[i]);
//	i--;
  //    }
      putch('E');
    }
    sleep();
    count++;
    if(count % 4 == 0) {
      meterValue++;
      count = 0;
    }
  }
} // main


static void sleep() {
  unsigned int counter1;
  unsigned int counter2;
  
  counter1 = 0xFFFF;  
  while(counter1 > 0) {
    counter1--;
    counter2 = 0xFFFF;
  }
}
