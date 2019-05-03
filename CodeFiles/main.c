#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include "io.c"
#include "timer.h"
#define button (~PINB & 0x01)
#define touchpad (~PINB & 0x02)

////////////////////////////////////////////////////////////////////////////////
//Functionality - Sets bit on a PORTx
//Parameter: Takes in a uChar for a PORTx, the pin number and the binary value 
//Returns: The new value of the PORTx
unsigned char SetBit(unsigned char pin, unsigned char number, unsigned char bin_value) 
{
    return (bin_value ? pin | (0x01 << number) : pin & ~(0x01 << number));
}

////////////////////////////////////////////////////////////////////////////////
//Functionality - Gets bit from a PINx
//Parameter: Takes in a uChar for a PINx and the pin number
//Returns: The value of the PINx
unsigned char GetBit(unsigned char port, unsigned char number) 
{
    return ( port & (0x01 << number) );
}

//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
    unsigned long int c;
    while(1){
        c = a%b;
        if(c==0){return b;}
        a = b;
b = c;
    }
    return 0;
}
//--------End find GCD function ----------------------------------------------

/////////////////////
// END OF INCLUDES //
/////////////////////


void InitADC() {
    ADMUX=(1<<REFS0);                   //setting the reference of ADC such THAT Areff=Avcc
    ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}
//used something new datatype her to check if it works or not, unsigned integer short can aolso be written as unint16_t

uint16_t ReadADC(uint8_t ch)
{   InitADC();
    ch=ch&0b00000111; //im using this channel to be a extra safety net for the correct reading
    ADMUX|=ch; // it will read from the first three channels
    //conversion steps
    ADCSRA|=(1<<ADSC);
    while(!(ADCSRA & (1<<ADIF)));
    ADCSRA|=(1<<ADIF); // got from one of the labs
    return(ADC);
}

//code for reading which direction is the joystick pointing to0
uint16_t joyStickFlicked(uint16_t x,uint16_t y)
{
    //1-> up, 2-> right, 3-> down 4-> left
    if(x>255 && x<765 && y>765) // we know 1024 is the max value, these values are exact half of the max values which i had using the joystick
    return(1); // direction is up

    else if(x>765 && y>255 && y<765)
    return(2); //direction is right

    else if(x>255 && x<765 && y<255)
    return(3); //direction is down

    else if(x<255 && y>255 && y<765)
    return(4); // direction is left

    else
    return(0); //default state
}

unsigned char checkInput(unsigned char stateNum){
    unsigned char valid = (0x01 << stateNum);
    unsigned char inputs = 0;
    unsigned short x,y;

    x = ReadADC(0);
    y = ReadADC(1);

    if (joyStickFlicked(x,y) != 0) { // joystick flicked
        inputs = (inputs << 1);
        inputs += 1;
    }
    else {
        inputs = (inputs << 1);
    }
    if (touchpad) { // touchpad hit
        inputs = (inputs << 1);
        inputs =+ 1;
    }
    else {
        inputs = (inputs << 1);
    }
    if (button){ // button hit
        inputs = (inputs << 1);
        inputs += 1;
    }
    else {
        inputs = (inputs << 1);
    }
    if (inputs == 0){ // nothing clicked
        return 0;
    }
    else if (inputs == valid) { // correctly pressed
        return 1; // correct
    }
    else { // wrong pressed
        return 2; // wrong
    }
}

typedef struct _task {
    int state;
    unsigned long period;
    unsigned long elapsedTime;
    int(*TickFct)(int);
} task;

enum GameStates{gameInit, gameStart, pushIt, bopIt, flickIt, endGame} GameState;
enum InputStates{InputStart, InputDown} InputState;

unsigned char waitForInputUp;

unsigned char joystick = 0;
signed short counter;
signed short delay;

int GameTick()
{
    switch(GameState) // Transitions
    {
        case gameInit:
        if (touchpad){ // start game with touchpad
            waitForInputUp = 0;
            GameState = gameStart;
            delay = 1000;
        }
        else {
            GameState = gameInit;
        }
        break;
        
        ////////// GAME START //////////
        
        case gameStart:
        if(!waitForInputUp)
        {
            delay-=20;
            counter = delay;
            switch(rand() % 3){
                case 0:
                GameState = pushIt;
                break;
                case 1:
                GameState = bopIt;
                break;
                case 2:
                GameState = flickIt;
                break;
                default:
                break;
            }
        }
        break;
        
        /////////// PUSH IT ///////////
        
        case pushIt:
        if (counter-- <= 0){ // nothing is clicked in 1s
            GameState = endGame;
        }else
        switch(checkInput(0)) { // state 0 = BUTTON
            case 0: // nothing pressed, waits for counter 
            GameState = pushIt;
            break;
            case 1: // ACCEPT
            GameState = gameStart;
            break;
            case 2: // REJECT 
            GameState = endGame;
            break;
            default:
            break;
        }
        break;
        
        /////////// BOP IT ////////////
        
        case bopIt:
        if (counter-- <= 0){ // nothing is clicked in 1s
            GameState = endGame;
        }else
        switch(checkInput(1)) { // state 0 = BUTTON
            case 0: // nothing pressed, waits for counter 
            GameState = bopIt;
            break;
            case 1: // ACCEPT
            GameState = gameStart;
            break;
            case 2: // REJECT 
            GameState = endGame;
            break;
            default:
            break;
        }
        break;
        
        /////////// FLICK IT ////////////

        case flickIt:
        if (counter-- <= 0){ // nothing is clicked in 1s
            GameState = endGame;
        }else
        switch(checkInput(2)) { // state 0 = BUTTON
            case 0: // nothing pressed, waits for counter 
            GameState = flickIt;
            break;
            case 1: // ACCEPT
            GameState = gameStart;
            break;
            case 2: // REJECT 
            GameState = endGame;
            break;
            default:
            break;
        }
        break;

        /////////// ENDGAME ////////////

        case endGame:
        if (counter++ == 3000){ // marker1
            GameState = gameInit;
            counter = delay;
        }
        break;

        default:
        break;
    }
    
    switch(GameState) // Actions
    {
        case gameInit:
        PORTC = 0;
        break;
        case gameStart:
        PORTC = 0;
        break;
        case pushIt:
        PORTC = 4;
        break;
        case bopIt:
        PORTC = 2;
        break;
        case flickIt:
        PORTC = 1;
        break;
        case endGame:
        PORTC = 8;
        default:
        break;
    }
    return GameState;
}

int InputTick()
{
    switch(InputState) // Transitions
    {
        case InputStart:
            if(checkInput(0) ||checkInput(1) || checkInput(2))
            {
                InputState = InputDown;
            }
            break;
        case InputDown:
            if(!(checkInput(0) ||checkInput(1) || checkInput(2)))
            {
                InputState = InputStart;
            }
            break;
        default:
            break;
    }
    switch(InputState) // Actions
    {
        case InputStart:
            waitForInputUp = 0;
            break;
        case InputDown:
            waitForInputUp = 1;
            break;
        default:
            break;
    }
    return InputState;
}
int main(void)
{
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0x00; PORTB = 0xFF; // PORTB set to output, outputs init 0s
    DDRC = 0xFF; PORTC = 0x00; // PC7..4 outputs init 0s, PC3..0 inputs init 1s
    DDRD = 0xFF; PORTD = 0x00; 

    static task GameTask, InputTask;
    task *tasks[] = { &GameTask, &InputTask };
    const unsigned short NumTasks = sizeof(tasks) / sizeof(task*);

    unsigned long int GamePeriod = 1;
    unsigned long int InputPeriod = 1;

    unsigned long int tmpGCD = 1; // when more than 1 task don't set to 1

    tmpGCD = findGCD(GamePeriod, InputPeriod);
    
    GamePeriod /= tmpGCD;
    InputPeriod /= tmpGCD;

    unsigned char i = 0;
    tasks[i]->state = gameInit;
    tasks[i]->period = GamePeriod;
    tasks[i]->elapsedTime = tasks[i]->period;
    tasks[i]->TickFct = &GameTick;
    i++;
    tasks[i]->state = InputStart;
    tasks[i]->period = InputPeriod;
    tasks[i]->elapsedTime = tasks[i]->period;
    tasks[i]->TickFct = &InputTick;
    i++;

    TimerSet(1);
    TimerOn();
    while(1) {
        for(int i = 0; i < NumTasks; i++)
        {
            if(tasks[i]->elapsedTime == tasks[i]->period)
            {
                tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
                tasks[i]->elapsedTime = 0;
            }
            tasks[i]->elapsedTime += 1;
        }
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 0;
}