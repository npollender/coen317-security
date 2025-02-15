#include <hidef.h>
#include "derivative.h"
#include "mixasm.h"
#include "lcd_commands.h"

void cmd2LCD (char cmd);
void openLCD(void);
void putcLCD(char cx);
void putsLCD_fast(char *ptr);
void putsLCD(char *ptr); 
char checkKeyPad(void);
void checkPassword(void);
void wrongPassword(void);
void setPassword(void);

char* msg;
char* password;
char* password_check;
char temp = 0x00;
char count = 0x00;
char count_check =0x00;
char attempts = 3;
char Locked = 0;
   
   
void main(void) 
{
  //Set Port E as output (Some bits are used by the keypad)
  DDRE = 0x00;
  
  //Set Bit 0 of Port H as an input for the enter button
  DDRH = 0x00;
  
	//Configure LCD
	openLCD();
	cmd2LCD(CLR_LCD);
	
	//Start Application
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
	
  setPassword();
		
	while(1) 
	{ 
	  if(!Locked)
	  {
	    cmd2LCD(CLR_LCD);
	    putsLCD(" Press 1 to Lock");
	    cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
	    putsLCD("Press 2 to chg pass");
	    Locked = 2;
	  } 
	  else if(Locked == 2) 
	  {
	     temp = checkKeyPad();
	     
	     if(temp == 0x01)
	      Locked = 4;
	     if(temp == 0x02) 
	     {
	        setPassword();
	        Locked = 0;
	     }
	  }
	  else 
	  {
	  
	  if(Locked != 3) 
	  {
	    putsLCD(" Locking Device");
      Locked = 0; //Lock state = 1
      asm_mydelay1ms(200);
      putsLCD(".");
      asm_mydelay1ms(200);
      putsLCD(".");
      asm_mydelay1ms(200);
      putsLCD(".");
	    cmd2LCD(CLR_LCD);
	    putsLCD(" Enter Password:");
    	cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
	    temp = 0x00;
	    Locked = 3;
	    count_check = 0;
	  }
	   
	  //Lock up
	  if(attempts == 0) 
	  {
	    while(1);
	  }
	  
	  temp = checkKeyPad(); 
    if(temp != 0x0f && temp != 0x0A) 
    {
      *(password_check+count_check) = temp;
      count_check++;
     if (temp == 0x01) {
      msg = "1"; 
     }if
     (temp == 0x02){
     msg = "2";
     }if
     (temp == 0x03){
      msg = "3";
     }if
     (temp == 0x04){
     msg = "4";

     }if
     (temp == 0x05){
     msg = "5";
     }if
     (temp == 0x06){
     msg = "6";
     }if          
     (temp == 0x07){
     msg = "7";
     }if
     (temp == 0x08){
     msg = "8";
     }if
     (temp == 0x09){
     msg = "9";
     }
     if
     (temp == 0x00){
     msg = "0";
     }
      putsLCD(msg);  
    }
    
    /*
    * User pressed Enter. Verify if password matches.
    */
    if(temp == 0x0A) 
    {
      checkPassword();
    }
	}
	}
}


void cmd2LCD (char cmd) 
{
    
    char hnibble, lnibble;
    // note that here, RS is always 0
    
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
//Button 1
    if(!PTH_PTH6) 
    {
      while(!PTH_PTH6);
      return 0x01;
    }
    // Button 2
    if(!PTH_PTH7) 
    {
      while(!PTH_PTH7);
      return 0x02;
    }
    //Button 3
    if(!PTH_PTH5) 
    {
      while(!PTH_PTH5);
      return 0x03;
    }
    //Button 4
    if(!PTH_PTH4) 
    { 
      while(!PTH_PTH4);
      return 0x04;
    }
    //Button 5
    if(!PORTE_BIT7) 
    {
      while(!PORTE_BIT7);
      return 0x05;
    }
    //Button 6
    if(!PORTE_BIT4) 
    {
      while(!PORTE_BIT4);
      return 0x06;
    }
    //Button 7
    if(!PTH_PTH2) 
    {
    while(!PTH_PTH2);
      return 0x07;
    }
    //Button 8
    if(!PTH_PTH3) 
    {
    while(!PTH_PTH3);
      return 0x08;
    }
    //Button 9
    if(!PTH_PTH1) 
    {
    while(!PTH_PTH1);
      return 0x09;
    }
    //Button 0
    if(!PORTE_BIT0) 
    {
    while(!PORTE_BIT0);
      return 0x00; 
    }
    //Enter button
    if(!PTH_PTH0) 
    {
      while(!PTH_PTH0);
      return 0x0A;
    }
    return 0x0f;
}

void checkPassword(void) {
      // Before comparing, check if length of password
      // is the same. If not, password entered is wrong
      if(count != count_check) 
      {
        wrongPassword();    
      } 
      else 
      {
        for(temp = 0; temp < count; temp ++) {
         if(*(password+temp) != *(password_check+temp)) {
           wrongPassword();
           temp = count;
           break;
         }else
          continue;
        }
        
        cmd2LCD(CLR_LCD);
        putsLCD(" Correct!");
        cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);
        putsLCD("Unlocking...");
        asm_mydelay1ms(200);
        attempts = 3;
        Locked = 0;
      }  
}

void wrongPassword(void) {
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
}

void setPassword(void) 
{
 	temp = 0;
 	count = 0;
	cmd2LCD(CLR_LCD);
	msg = " Set a Password:";
	putsLCD(msg);
	cmd2LCD(MOVE_CURSOR_TO_2ND_ROW);

  // While loop prompting user to register a password	
	while(temp != 0x0A) 
	{ 
	  temp = checkKeyPad(); 
    if(temp != 0x0f && temp != 0x0A) 
    {
      *(password+count) = temp;
      count++;
      putsLCD("*");  
    }
	}
	
	cmd2LCD(CLR_LCD);
	msg = " Password Set.";
	putsLCD(msg);
	cmd2LCD(CLR_LCD); 
}


