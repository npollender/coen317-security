#include <hidef.h>
#include "derivative.h"
#include "mixasm.h"
#include "lcd_commands.h"


// Function Prototypes
void cmd2LCD (char cmd);
void openLCD(void);
void putcLCD(char cx);
void putsLCD_fast(char *ptr);
void putsLCD(char *ptr); 
char checkKeyPad(void);
void checkPassword(void);
void wrongPassword(void);
void setPassword(void);

// Application Variables
char* msg;
char* password;
char* password_check;
char temp = 0x00;
char count = 0x00;
char count_check =0x00;
char keypad_pin_toggle = 0;
char attempts = 3;
enum { INIT, LOCKED, SETTING, UNLOCKED, CHECK} state;
char bttn_ENTER_read = 0;


 // Interrupt Service Routine for PORTH button press
#pragma CODE_SEG NON_BANKED 
 interrupt void bttnHISR(void) 
 {
  
  // Disable Further Interrupts until this one is processed
  PIEH = 0x00;
  
  // Determine what action to take depending on the state of the
  // program
 }

// Interrupt Service Routine for timer overflow
// Cycles through the output modes of porth's 4 LSB's in order to detect button presses
interrupt void timerOverflow(void) 
{
  // Stop Counter
  TSCR1 = 0x00;
  // Disable Overflow interrupts
  TSCR2 &= 0x7F;
  switch(keypad_pin_toggle%4) 
  {
    case 0:
      DDRH = 0xFE;
      keypad_pin_toggle++;
      break;
    case 1:
      DDRH = 0xFD;
      keypad_pin_toggle++;
      break;
    case 2:
      DDRH = 0xFB;
      keypad_pin_toggle++;
      break;
    case 3:
      DDRH = 0xF7;
      keypad_pin_toggle = 0;
      break;
  }
  
  // Reset Counter Register
  TCNT = 0x0000;
  // Resume Counter
  TSCR1 = 0x80;
  // Enable Overflow Interrupts
  TSCR2 |= 0x80;
  // Clear the Overflow interrupt flag
  TFLG2 = 0xFF;
}


// Assign the ISR to the appropriate vector address for PORT H (0xFFCC) 
#pragma CODE_SEG DEFAULT
 typedef void (*near tIsrFunc)(void);
 const tIsrFunc _vect[] @0xFFCC = 
 { 
    /* Interrupt table */
    bttnHISR
 };  
 
 // Assign the ISR of the timer overflow to the appropriate vector address for the enhanced capture timer overflow (0xFFDE)
 const tIsrFunc _vect2[] @0xFFDE = 
 { 
    /* Interrupt table */
    timerOverflow
 };   

void main(void) 
{

	//Configure LCD
	openLCD();
	cmd2LCD(CLR_LCD);
	
	//Enable Interrupts
	EnableInterrupts;;
	
  // Set Half of PORT H pins as inputs and half as outputs.
  // This port is used by the keypad to generate interrupts on key press.
  // The reason for dividing half the port as input/output is to produce a matrix interconnect of the buttons
  // to utilize less pins for the amount of available buttons. Refer to the dragon-12 user guide for more info.
  DDRH = 0xF0;
  
  // Set all of PORT H's pins to send an interrupt signal whenever one of the pins receives a rising edges, signifying a button press.
  PPSH = 0x00;
  
  // Disable PORT H interrupts until further notice
  PIEH = 0x00;
  
  // Enable Interrupt on Timer Overflow (When TCNT goes from 0xFFFF to 0x0000)
  TSCR2 = 0x80;
  
  // Set the pre-scale value to 128 to avoid the Timer ISR consuming too much of the CPU's bandwidth
  TSCR2 |= 0x03;
  
  // Enable the Free-Running 16 bit internal timer module
  // The Timer will produce an interrupt everytime it overflows.
  // These interrupts will be used to alternate the high signal sent through the keypad's
  // Rows to be able to know which key has been pressed. (Again, refer to Dragon-12 Light User guide for more info)
  TSCR1 = 0x80;
  
 
  // Set initial State of Program
  state = INIT;
	
	// Application Start
	msg = " COEN317-Security";
	putsLCD(msg);
	asm_mydelay1ms(200);
	asm_mydelay1ms(200);
	asm_mydelay1ms(200);
	asm_mydelay1ms(200);
	asm_mydelay1ms(200);
	asm_mydelay1ms(200);
	asm_mydelay1ms(200);
	asm_mydelay1ms(200);
	cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
	msg = "Initializing |";
	putsLCD(msg);
	
	for(temp = 0; temp < 15; temp++) 
	{
	  if(temp%3 == 1)
	    msg = "Initializing /";
	  if(temp%3 == 2)
	    msg = "Initializing -";
	  if(temp%3 == 0)
	    msg = "Initializing |";
	  cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
	  putsLCD_fast(msg);
	  asm_mydelay1ms(200);
	  asm_mydelay1ms(200);  
	}
	
	cmd2LCD(CLR_LCD);
		
	// Main Program Loop	
	while(1) 
	{ 
    switch(state) 
    {
      case INIT:
        setPassword();
        break;
      case LOCKED:
        break;
      case CHECK:
        checkPassword();
        break;
      case UNLOCKED:
        break;
      default:
      break;
    }
	}
}


void cmd2LCD (char cmd) 
{
    
    char hnibble, lnibble;
    
    PORTK = 0x00; // EN, RS=0    

    // sending higher nibble        
    hnibble = cmd &0xF0;
    PORTK = 0x02; // EN = 1    
    hnibble >>=2; // shift two bits to the right to align with PORTK positions
    PORTK = hnibble|0x02;
    asm("nop"); // 1 cycle = 1/(24*10^6) = 41.66 ns
    asm("nop");
    asm("nop");
    PORTK = 0x00; // EN,RS=0    
    
    // Sending lower nibble:
    lnibble = cmd &0x0F;
    PORTK = 0x02; // EN = 1
    lnibble <<= 2; // shift two bits to the left to align with PORTK positions
    PORTK = lnibble|0x02;
    asm("nop");
    asm("nop");
    asm("nop");    
    PORTK = 0x00; // EN, RS = 0
    
    // Wait 50 us for order to complete (enough time for most IR instructions)
    asm_mydelay10us(5);
}

void openLCD(void) 
{
    DDRK = 0xFF; // PortK configured as outputs
    asm_mydelay1ms(100); // wait for 100 ms
    cmd2LCD(0x28); // set 4-bit data, 2-line display, 5x8 font
    cmd2LCD(0x0F); // turn on display, cursor, blinking
    cmd2LCD(0x06); // move cursor right
    cmd2LCD(0x80);  // set DDRAM address. DDRAM data are ent after this setting. address=0
    cmd2LCD(0x01); // clear screen, move cursor to home
    asm_mydelay1ms(2); // wait for 2 ms. The 'clear display' instruction requires this   
}

void putcLCD(char cx) 
{
    char hnibble, lnibble;
    //temp = cx;
    // note that here, RS is always 1
    
    PORTK = 0x01; // RS=1, EN=0
    
    // sending higher nibble
    hnibble = cx&0xF0;
    PORTK = 0x03; // RS=1, EN=1
    hnibble >>= 2; // shift two bits to the right to align with PORTK positions
    PORTK = hnibble|0x03;
    asm("nop");
    asm("nop");
    asm("nop");    
    PORTK = 0x01; // RS=1, E=0
    
    // sending lower nibble
    lnibble = cx & 0x0F;
    PORTK = 0x03; // RS=1, E=1
    lnibble <<= 2;  // shift two bits to the left to align with PORTK positions        
    PORTK = lnibble|0x03;
    asm("nop");
    asm("nop");
    asm("nop");    
    PORTK = 0x01;// RS=1, E=0
    
    // Wait 50 us for order to complete (enough time for most IR instructions)    
    asm_mydelay10us(5); // wait for 50  us
}

void putsLCD (char *ptr) 
{
    int count= 0;
    while (*ptr) 
    {
        putcLCD(*ptr);
        asm_mydelay1ms(80);
        if(count > 16)
          cmd2LCD(SHIFT_DISP_RIGHT);
        ptr++;
        count++;
    }
}

void putsLCD_fast(char *ptr) 
{
    while (*ptr) 
    {
        putcLCD(*ptr);
        ptr++;
    }
}

// This function verifies if the keypad has been pressed.
// If it was pressed, then the return value represents the key pressed.
// If no key pressed, then the return value is 0x0F
// If enter was pressed, value of 0x0A is returned
char checkKeyPad(void) 
{
    if(PIFH_PIFH0)
    return 0x0f;
}

void checkPassword(void) 
{
      // Before comparing, check if length of password
      // is the same. If not, password entered is wrong
      if(count != count_check) 
      {
        wrongPassword();    
      } 
      else 
      {
        for(temp = 0; temp < count; temp ++) 
        {
         if(*(password+temp) != *(password_check+temp)) 
         {
           wrongPassword();
           temp = count;
           break;
         }
         else
          continue;
        }
        
        cmd2LCD(CLR_LCD);
        putsLCD(" Correct!");
        cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
        putsLCD("Unlocking...");
        asm_mydelay1ms(200);
        attempts = 3;
        state = UNLOCKED;
      }  
}

void wrongPassword(void) 
{
  cmd2LCD(CLR_LCD);
  putsLCD(" Wrong Password.");
  cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
  switch(attempts) {
    case 3:
    msg = "2 Tries Left.";
    break;
    case 2:
    msg = "1 Try Left.";
    break;
    case 1:
    msg = "No Attempts Left.";
    break;
    default:
    break;
   }
   putsLCD(msg);
   attempts--;
   asm_mydelay1ms(200);
   asm_mydelay1ms(200);
   cmd2LCD(CLR_LCD);
   if(attempts !=0)
    putsLCD(" Enter Password:");
   else {
    putsLCD(" Device Permanently");
    cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
    putsLCD("Locked.");
   }
   cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
   state = LOCKED;
}

void setPassword(void) 
{
   putsLCD(" Enter a Password");
   cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
   PIEH = 0x0F;
   state = SETTING;
}


