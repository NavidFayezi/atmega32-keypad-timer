
/*
password is 4445
*/

#include <io.h>
#include <mega32.h>
#include <alcd.h>
#include <stdlib.h>
#include <stdio.h>
#include <delay.h>


unsigned char EEPROM_read(unsigned char uiAddress);
void EEPROM_write(unsigned char uiAddress, unsigned char ucData);


enum SYSTEM_STATE {UNLOCKED , LOCKED, SETPASSWORD };
enum SYSTEM_STATE systemstate  = SETPASSWORD ;


int s = 0;
int m = 0;
int h = 0;

char buffer[16];
int flag = 0;

unsigned char keypadPatterns[12] = {0x7d,0xbd,0xdd,0xed,0x7b,0xbb,0xdb,0xeb,0x77,0xb7,0xd7,0xe7}; // 10 -> * , 11 -> #

int new_key;
int pressed_key;

void main(void)
{

   
    
    // EEPROM_write(100,0xff);                       //uncomment this line to set a new password
    unsigned char password_length;
    unsigned char password [10];                     // digits of password are stored in this array.
    int i;
    

    
    lcd_init(40);
    lcd_clear();                                    // lcd initial settings
    
    if(EEPROM_read(100) == 0){                      // check to see if device has a password.

        systemstate = LOCKED; 
        password_length = EEPROM_read(101);

        for(i =0 ; i < password_length ; i++){
            password[i] = EEPROM_read(102+i);
            }
            
        }
    
    DDRC = 0xf0;                                   // port C settings, connected to keypad
    PORTC = 0x0e;                                    
    
                                                   
    DDRB.2 = 0;                                    // INT2 port
    PORTB.2 = 1;
       
    GICR = 1 << INT2;                              // enable INT2
    MCUCSR &= ~(1 << ISC2);                        // falling edge for INT2
    
    lcd_gotoxy(0,1);
    sprintf(buffer, "%d%d:%d%d:%d%d" ,h/10,h%10,m/10,m%10,s/10,s%10);  // initialize lcd
    lcd_puts(buffer);
    
    
   
    
    OCR1AH = 0x3D;
    OCR1AL = 0x09;                                  // OCR1A = ox3D09 = 15625 --> 15625 * 64 / 1000000 = 1 second 
    
    #asm("sei");                                    // globally enable interrupts
    TIMSK |= (1 << TOIE1) | (1 << OCIE1A) ;         // enable timer/counter 1 interrupt and timer/counter 1 compare match interrupt  
    
    TCCR1B = 0x0B;                                  // f(t/c) = f(IO) / 64 from prescaler || enable CTC mode and set TOP to OCR1A value
      
    
    DDRB.0 = DDRB.1 = 1;                            // pin.0 for yellow LED and pin.1 for green LED.

                
    DDRD.2 = DDRD.3 = 0; 
    PORTD.2 = PORTD.3 = 1;                          //pull up
    
    MCUCR |= 0<<ISC00;
    MCUCR |= 1<<ISC01;                              // falling edge.
    GICR |= 1<<INT0;                                // enable INT0 
    
    MCUCR |= 0<<ISC10;
    MCUCR |= 1<<ISC11;                              // falling edge
    GICR |= 1<<INT1;                                // enable INT1


    while(1){
        if(systemstate == LOCKED){
            char stars[10] = {0};
            char temp[5];
            int wrong_pass = 0;
            lcd_clear();
            lcd_gotoxy(0,0);
            lcd_putsf("Locked. Enter your password:");
            for(i = 0; i<password_length; i++){
                 
                 while(new_key == 0 );
                 new_key = 0;
                 lcd_gotoxy(0,1);
                 lcd_puts(stars);
                 stars[i]='*';
                 itoa(pressed_key,temp);
                 lcd_puts(temp);
                 if(pressed_key != password[i])
                    wrong_pass = 1;
                 
                }
                
                if(wrong_pass == 0)
                    systemstate = UNLOCKED;
            }
        if(systemstate == UNLOCKED){
            lcd_clear();
            lcd_putsf("Unlocked");        
        }
        
        if(systemstate == SETPASSWORD){
            char temp[5];
            char stars[10];
            lcd_clear();
            lcd_gotoxy(0,0);
            lcd_putsf("set password(4-9 digits):");
            while (new_key ==0);
            new_key =0;
            password_length = pressed_key % 10;
            
            if(password_length < 4)                         // minimum length of password is 4 digits.
                password_length = 4;

            
            itoa(password_length,temp);
            lcd_gotoxy(0,0);
            lcd_putsf("Enter your password(");
            lcd_puts(temp);
            lcd_putsf(" digits):");
            for(i = 0; i<password_length; i++){
                 
                 while(new_key == 0 );
                 new_key = 0;
                 password[i] = pressed_key;
                 lcd_gotoxy(0,1);
                 lcd_puts(stars);
                 stars[i]='*';
                 itoa(pressed_key,temp);
                 lcd_puts(temp);
                 
                }
            #asm("cli");                                    // disable interrupts during eeprom write
            EEPROM_write(100,0);
            EEPROM_write(101,password_length);
            
            for (i =0 ; i< password_length ; i++)
                EEPROM_write(102+i,password[i]);
            #asm("sei");
            systemstate = LOCKED ;
            lcd_clear();
            lcd_gotoxy(0,0);
            lcd_putsf("Password is set, press any key ...");
            while ( new_key == 0);
            new_key = 0; 
        }
    
    
    }                                  
    
    
}


interrupt[TIM1_COMPA] void comparematch(void){                      // interrupt happens every second
    
   
    
        
    if (flag == 0){
        PORTB.0 = 0;
        PORTB.1 = 0;
        //Second 
        s = s + 1;
        if (s == 60) {
            s = 0;
            m++;
            PORTB.0 = 1;
        }
        //Min
        if (m == 60 ) {
            m = 0;
            h++;
            PORTB.1 = 1;
        }
        //Hour
        if (h == 24){
            h = 0;
        }
    }
    
    lcd_gotoxy(0,1);
    sprintf(buffer, "%d%d:%d%d:%d%d" ,h/10,h%10,m/10,m%10,s/10,s%10);
    // lcd_puts(buffer);    
     
}

interrupt [EXT_INT0] void onPause (void){
    if (flag == 1) flag = 0;
    else if (flag == 0) flag = 1; 
}

interrupt [EXT_INT1] void onReStart (void){
    s = 0;
    m = 0;
    h = 0;       
}


interrupt [EXT_INT2] void keyPressed(void){
   

    
   // find key 
    int i;
    unsigned char pattern;  
    // delay_ms(20);   debounce
    
    new_key = 1;                                           
  
    
    DDRC |= 0xf0;                                                   // leaves portc.0 unchanged. portc.0 is connected to sensor and used by ADC.
    DDRC &= 0xf1;                                                   // same as above
    PORTC &= 0x0f;                       
    PORTC |= 0x0e;                      
    delay_us(5);
    pattern = (PINC & 0b00001111); 
    DDRC |= 0x0e;                        
    DDRC &= 0x0f;
    PORTC |= 0xf0;
    PORTC &= 0xf1; 
    delay_us(5); 
    pattern |= (PINC & 0b11110000) | 0x01;                          // lsb is not connected to keypad, always gets the value 1
    for(i = 0 ; i < 12 ; i++){
        if(keypadPatterns[i] == pattern){
            pressed_key = i;
            break;
            }
    
    } 
    
    
    
   
    
}


void EEPROM_write(unsigned char uiAddress, unsigned char ucData){    // address is in 0-255 range
    /* Wait for completion of previous write */
    while(EECR & (1<<EEWE));
    /* Set up address and data registers */
    EEARH = 0;
    EEARL = uiAddress;
    
    EEDR = ucData;
    /* Write logical one to EEMWE */
    EECR |= (1<<EEMWE);
    /* Start eeprom write by setting EEWE */
    EECR |= (1<<EEWE);
}


unsigned char EEPROM_read(unsigned char uiAddress){     // address is in 0-255 range
    /* Wait for completion of previous write */
    while(EECR & (1<<EEWE));
    /* Set up address register */
    EEARH = 0;
    EEARL = uiAddress;
    /* Start eeprom read by writing EERE */
    EECR |= (1<<EERE);
    /* Return data from data register */
    return EEDR;
}