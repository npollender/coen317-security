#include <hidef.h>
#include "derivative.h"
#include "mixasm.h"
#include "lcd_commands.h"

#define MAX_LENGTH 16


/*
*  Function Prototypes
*/
void cmd2LCD (char cmd);
void openLCD(void);
void putcLCD(char cx);
void putsLCD_fast(char *ptr);
void putsLCD(char *ptr); 
void checkKeyPad(void);
void checkPassword(void);
void wrongPassword(void);
void setPassword(void);
void lockDevice(void);

/*
*  Application Variables
*/
char* msg;
char password[MAX_LENGTH];
char password_check[MAX_LENGTH];
char temp;
char count;
char count_check;
char it = 0x00;
char attempts = 3;
enum { INIT, LOCKED, SETTING, UNLOCKED, CHECK, SET} state;


/**********************************************************
*  
* ISR for button press
*
***********************************************************/
#pragma CODE_SEG NON_BANKED 
 interrupt void bttnHISR(void) 
 {
 
  // Disable Further Interrupts until this one is processed
  PIEH = 0x00;
  
  // Determine what action to take depending on the state of the program
  checkKeyPad();
  
  asm_mydelay1ms(25);
  
  // Clear Interrupt Flags
  PIFH = 0xFF;
  
  // Enable Interrupts
  PIEH = 0xFF;
 }

/**********************************************************
*  
* ISR for Timer Overflow (Not Used)
*
***********************************************************/
/*
interrupt void timerOverflow(void) 
{
  // Stop Counter
  TSCR1 = 0x00;
  // Disable Overflow interrupts
  TSCR2 &= 0x7F;
  
  PTH &= 0xF0;
  
  switch(keypad_pin_toggle%20) 
  {
    case 0:
      DDRH = 0x01;
      active = COL0;
      keypad_pin_toggle++;
      break;
    case 5:
      DDRH = 0x02;
      active = COL1;
      keypad_pin_toggle++;
      break;
    case 10:
      DDRH = 0x04;
      active = COL2;
      keypad_pin_toggle++;
      break;
    case 15:
      DDRH = 0x08;
      active = COL3;
      keypad_pin_toggle = 0;
      break;
    default:
      keypad_pin_toggle++;
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
*/


/**********************************************************
*  
* Assign ISR function pointer to the dedicated vector address
* for port H. (Address 0xFFCC)
*
***********************************************************/
#pragma CODE_SEG DEFAULT
 typedef void (*near tIsrFunc)(void);
 const tIsrFunc _vect[] @0xFFCC = 
 { 
    bttnHISR
 };  
 
/**********************************************************
*  
* Assign ISR function pointer to the dedicated vector address
* for timer overflow. (Address 0xFFDE) (Not Used)
*
***********************************************************/
 /*
 const tIsrFunc _vect2[] @0xFFDE = 
 { 
    timerOverflow
 }; 
 */  

/**********************************************************
*  
* Main Function
*
***********************************************************/
void main(void) 
{

	//Configure LCD
	openLCD();
	cmd2LCD(CLR_LCD);
	
	//Enable Interrupts
	EnableInterrupts;;
	
  // Set Port H pins as outputs. When a button press occurs, it grounds the corresponding
  // pin and sends an interrupt to the cpu.
  DDRH = 0x00;
  
  // Set all of PORT H's pins to send an interrupt on falling edge (from 1 to 0)
  PPSH = 0xFF;
  
  // Disable PORT H interrupts until further notice
  PIEH = 0x00;
  
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
	
	for(it = 0; it < 15; it++) 
	{
	  if(it%3 == 1)
	    msg = "Initializing /";
	  if(it%3 == 2)
	    msg = "Initializing -";
	  if(it%3 == 0)
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
        PIEH = 0x00;
        checkPassword();
        PIEH = 0xFF;
        break;
      case UNLOCKED:
        break;
      case SET:
        lockDevice();
      default:
      break;
    }
	}
}

/**********************************************************
*  
* Send a command to the LCD
*
***********************************************************/
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

/**********************************************************
*  
* Initialize LCD (Port K)
*
***********************************************************/
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

/**********************************************************
*  
* Write a single character to LCD at current cursor position
*
***********************************************************/
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

/**********************************************************
*  
* Write a string to the LCD (with delay)
*
***********************************************************/
void putsLCD (char *ptr) 
{
    int c_count= 0;
    while (*ptr) 
    {
        putcLCD(*ptr);
        asm_mydelay1ms(80);
        if(c_count > 16)
          cmd2LCD(SHIFT_DISP_RIGHT);
        ptr++;
        c_count++;
    }
}

/**********************************************************
*  
* Write a string to the LCD (without delay)
*
***********************************************************/
void putsLCD_fast(char *ptr) 
{
    while (*ptr) 
    {
        putcLCD(*ptr);
        ptr++;
    }
}

/**********************************************************
*  
* Check which key on the keypad was pressed. 
* Only accessed by the button press ISR of Port H.
*
***********************************************************/
void checkKeyPad(void) 
{
if(state == SETTING) 
{
    if(count == (MAX_LENGTH + 1)) {
      return;        
    }
    
  if(PIFH_PIFH4)  // Pressed 0
  {
      password[count] = 0x00;
      count++;
      putsLCD("0");
  } 
  if(PIFH_PIFH6)  // Pressed 1
  {
      password[count] = 0x01;
      count++;
      putsLCD("1");
  }
  if(PIFH_PIFH5)  // Pressed 2
  {
      password[count] = 0x02;
      count++;
      putsLCD("2");
  }
  if(PIFH_PIFH7)  // Pressed 3
  {
      password[count] = 0x03;
      count++;
      putsLCD("3");
  }
  if(PIFH_PIFH0)  // Pressed 4
  {
      password[count] = 0x04;
      count++;
      putsLCD("4");
  }
  if(PIFH_PIFH2)  // Pressed 5
  {
      password[count] = 0x05;
      count++;
      putsLCD("5");
  }
  if(PIFH_PIFH3)  // Pressed 6
  {
      password[count] = 0x06;
      count++;
      putsLCD("6");
  }
  if(PIFH_PIFH1)  // Pressed Enter
  {
    count--;
    state = SET;
  }
}
else if(state == LOCKED) 
{
    if(count_check == (MAX_LENGTH + 1))
      return;
    
  if(PIFH_PIFH4)  // Pressed 0
  {
      password_check[count_check] = 0x00;
      count_check++;
      putsLCD("0");
  } 
  if(PIFH_PIFH6)  // Pressed 1
  {
      password_check[count_check] = 0x01;
      count_check++;
      putsLCD("1");
  }
  if(PIFH_PIFH5)  // Pressed 2
  {
      password_check[count_check] = 0x02;
      count_check++;
      putsLCD("2");
  }
  if(PIFH_PIFH7)  // Pressed 3
  {
      password_check[count_check] = 0x03;
      count_check++;
      putsLCD("3");
  }
  if(PIFH_PIFH0)  // Pressed 4
  {
      password_check[count_check] = 0x04;
      count_check++;
      putsLCD("4");
  }
  if(PIFH_PIFH2)  // Pressed 5
  {
      password_check[count_check] = 0x05;
      count_check++;
      putsLCD("5");
  }
  if(PIFH_PIFH3)  // Pressed 6
  {
      password_check[count_check] = 0x06;
      count_check++;
      putsLCD("6");
  }
  if(PIFH_PIFH1)  // Pressed Enter
  {
    count_check--;
    state = CHECK;
  }
} 
  else if(state == UNLOCKED) 
  {
    if(PIFH_PIFH6)  // Pressed 1
    {
      state = SET;
    }
    if(PIFH_PIFH5)  // Pressed 2
    {
      state = INIT;
    }
  }
}

/**********************************************************
*  
* Verify if entered password matches stored password
*
***********************************************************/
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
        for(temp = 0; temp <= count; temp ++) 
        {
         if(password[temp] != password_check[temp]) 
         {
           wrongPassword();
           return;
         }
        }
        
        cmd2LCD(CLR_LCD);
        putsLCD(" Correct!");
        cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
        putsLCD("Unlocking...");
        asm_mydelay1ms(200);
        attempts = 3;
        state = UNLOCKED;
        asm_mydelay1ms(200);
        cmd2LCD(CLR_LCD);
        putsLCD(" Main Menu");
        asm_mydelay1ms(200);
        asm_mydelay1ms(200);
        cmd2LCD(CLR_LCD);
        putsLCD(" 1 - Lock Device");
        cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
        putsLCD("2 - Set Password");
      }  
}

/**********************************************************
*  
* Actions to take if password entered is wrong
*
***********************************************************/
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
   else 
   {
    putsLCD(" Device Locked");
    cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
    putsLCD("Permanently.");
   }
   cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
   state = LOCKED;
   count_check = 0;
}

/**********************************************************
*  
* Actions to take when user wishes to set a new password
*
***********************************************************/
void setPassword(void) 
{
   cmd2LCD(CLR_LCD);
   putsLCD(" Enter a Password");
   cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
   count = 0;
   PIEH = 0xFF;
   state = SETTING;
}

/**********************************************************
*  
* Actions to take to lock the device
*
***********************************************************/
void lockDevice(void) 
{
   PIEH = 0x00;
   cmd2LCD(CLR_LCD);
   putsLCD(" Locking Device...");
   asm_mydelay1ms(200);
   state = LOCKED;
   cmd2LCD(CLR_LCD);
   putsLCD(" Enter Password");
   cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
   count_check = 0;
   PIEH = 0xFF;
}


