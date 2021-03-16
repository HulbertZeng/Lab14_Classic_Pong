 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #14  Exercise #1
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: https://youtu.be/x3kW_hsmN3c
 */ 
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include "timer.h"
#include "scheduler.h"

void A2D_init() {
      ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
    // ADEN: Enables analog-to-digital conversion
    // ADSC: Starts analog-to-digital conversion
    // ADATE: Enables auto-triggering, allowing for constant
    //        analog to digital conversions.
}

void transmit_data(unsigned char data, unsigned char gnds) {
    int i;
    for (i = 0; i < 8 ; ++i) {
         // Sets SRCLR to 1 allowing data to be set
         // Also clears SRCLK in preparation of sending data
         PORTC = 0x08;
         PORTD = 0x08;
         // set SER = next bit of data to be sent.
         PORTC |= ((data >> i) & 0x01);
         PORTD |= ((gnds >> i) & 0x01);
         // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
         PORTC |= 0x02;
         PORTD |= 0x02;
    }
    // set RCLK = 1. Rising edge copies data from â€œShiftâ€ register to â€œStorageâ€ register
    PORTC |= 0x04;
    PORTD |= 0x04;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
    PORTD = 0x00;
}

// Pins on PORTA are used as input for A2D conversion
    //    The default channel is 0 (PA0)
    // The value of pinNum determines the pin on PORTA
    //    used for A2D conversion
    // Valid values range between 0 and 7, where the value
    //    represents the desired pin for A2D conversion
void Set_A2D_Pin(unsigned char pinNum) {
    ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
    // Allow channel to stabilize
    static unsigned char i = 0;
    for ( i=0; i<15; i++ ) { asm("nop"); } 
}


// shared task variables
unsigned char game_over = 0;
unsigned char p_score = 0;
unsigned char e_score = 0;

// player paddle control
enum player { p_wait, p_left, p_right };
// task variables
unsigned char p_row = 0x8F;
const unsigned char p_pattern = 0x80;
unsigned char p_mid = 0xDF;

int player(int state) {
    unsigned short joystick = ADC;
    static signed char p_y = 0;
    switch(state) {
        case p_wait:
            if(joystick < 500) {
                state = p_left;
            }
            if(joystick > 600) {
                state = p_right;
            }
            break;
        case p_left:
            if(p_y < 1) {
                p_row = (p_row >> 1) | 0x80;
                p_mid = (p_mid >> 1) | 0x80;
                ++p_y;
            }
            state = p_wait;
            break;
        case p_right:
            if(p_y > -1) {
                p_row = (p_row << 1) | 0x01;
                p_mid = (p_mid << 1) | 0x01;
                --p_y;
            }
            state = p_wait;
            break;
        default: state = p_wait; break;
    }

    return state;
}


// enemy or obstacle control and positions
enum enemy { e_wait, e_left, e_right };
// task variables
unsigned char e_row = 0x00;
const unsigned char e_pattern = 0x01;
unsigned char e_mid = 0xDF;

int enemy(int state) {
    switch(state) {}

    return state;
}


// ball position, speed, and direction
enum ball { start, leftup, leftdown, rightdown, rightup, end };
// task variables
unsigned char b_row = 0x00;
unsigned char b_pattern = 0x00;
unsigned char b_speed = 4;
unsigned char b_delay = 4;
unsigned char b_spin = 1;

int ball(int state) {
    static signed char b_y = 0;
    unsigned short joystick = ADC;
    switch(state) {
        case start:
            b_row = 0xDF;
            b_pattern = 0x08;
            state = rightup;
            break;
        case leftup:
            if(b_delay == 0) {
                b_delay = b_speed;
                if(b_y == 2) {
                    state = leftdown;
                } else if((b_pattern == 0x40) && (b_row == (b_row | p_row))) {
                    state = rightup;
                    if(joystick < 500 || joystick > 600) {
                        b_spin = 2;
                    } else {
                        b_spin = 1;
                    }
                    if(p_mid == b_row) {
                        ++b_speed;
                    } else if(b_speed > 0) {
                        --b_speed;
                    }
                } else if(b_pattern == 0x80) {
                    ++e_score;
                    game_over = 1;
                    state = end;
                } else {
                    if(b_spin == 1 || b_y == 1) {
                        ++b_y;
                        b_row = (b_row >> 1) | 0x80;
                    } else {
                        b_y += b_spin;
                        b_row = (b_row >> b_spin) | 0xC0;
                    }
                    b_pattern = b_pattern << 1;
                    state = leftup;
                }
            } else {
                --b_delay;
                state = leftup;
            }

            break;
        case leftdown:
            if(b_delay == 0) {
                b_delay = b_speed;
                if(b_y == -2) {
                    state = leftup;
                } else if((b_pattern == 0x40) && (b_row == (b_row | p_row))) {
                    state = rightdown;
                    if(joystick < 500 || joystick > 600) {
                        b_spin = 2;
                    } else {
                        b_spin = 1;
                    }
                    if(p_mid == b_row) {
                        ++b_speed;
                    } else if(b_speed > 0) {
                        --b_speed;
                    }
                } else if(b_pattern == 0x80) {
                    ++e_score;
                    game_over = 1;
                    state = end;
                } else {
                    if(b_spin == 1 || b_y == -1) {
                        --b_y;
                        b_row = (b_row << 1) | 0x01;
                    } else {
                        b_y -= b_spin;
                        b_row = (b_row << b_spin) | 0x03;
                    }
                    b_pattern = b_pattern << 1;
                    state = leftdown;
                }
            } else {
                --b_delay;
                state = leftdown;
            }

            break;
        case rightdown:
            if(b_delay == 0) {
                b_delay = b_speed;
                if(b_y == -2) {
                    state = rightup;
                } else if((b_pattern == 0x02) && (b_row == (b_row | e_row))) {
                    state = leftdown;
                    /*if(~PINB != 0) {
                        b_spin = 2;
                    } else {
                        b_spin = 1;
                    }*/
                    if(e_mid == b_row) {
                        ++b_speed;
                    } else if(b_speed > 0) {
                        --b_speed;
                    }
                } else if(b_pattern == 0x01) {
                    ++p_score;
                    game_over = 1;
                    state = end;
                } else {
                    if(b_spin == 1 || b_y == -1) {
                        --b_y;
                        b_row = (b_row << 1) | 0x01;
                    } else {
                        b_y -= 2;
                        b_row = (b_row << 2) | 0x03;
                    }
                    b_pattern = b_pattern >> 1;
                    state = rightdown;
                }
            } else {
                --b_delay;
                state = rightdown;
            }

            break;
        case rightup:
            if(b_delay == 0) {
                if(b_y == 2) {
                   state = rightdown;
                } else if((b_pattern == 0x02) && (b_row == (b_row | e_row))) {
                   state = leftup;
                    /*if(~PINB != 0) {
                        b_spin = 2;
                    } else {
                        b_spin = 1;
                    }*/
                    if(e_mid == b_row) {
                        ++b_speed;
                    } else if(b_speed > 0) {
                        --b_speed;
                    }
                } else if(b_pattern == 0x01) {
                   ++p_score;
                   game_over = 1;
                   state = end;
                } else {
                    if(b_spin == 1 || b_y == 1) {
                        ++b_y;
                        b_row = (b_row >> 1) | 0x80;
                    } else {
                        b_y += 2;
                        b_row = (b_row >> 2) | 0xC0;
                    }
                    b_pattern = b_pattern >> 1;
                    state = rightup;
                }
            } else {
                --b_delay;
                state = rightup;
            }

            break;
        case end:
            if(game_over) {
                state = end;
            } else {
                state = start;
            }
            break;
        default: state = start; break;
    }

    return state;
}


// displays player, enemy, and ball
enum game { display1, display2, display3, gameover };
// task variables

int game(int state) {
    switch(state) {
        case display1:
             transmit_data(p_pattern, p_row);
             state = display2;
             break;
        case display2:
             transmit_data(e_pattern, e_row);
             state = display3;
             break;
        case display3:
             transmit_data(b_pattern, b_row);
             if(game_over) {
                 state = gameover;
             } else {
                 state = display1;
             }
             break;
        case gameover:
             transmit_data(0x00, 0xFF);
             break;
        default: state = display1;
    }

    return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0x00; PORTB = 0xFF;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    /* Insert your solution below */
    static task task1, task2, task3, task4; //static task variables
    task *tasks[] = { &task1, &task2, &task3, &task4 };
    const unsigned short numTasks = sizeof(tasks)/sizeof(*tasks);
    const char start = -1;
    // player
    task1.state = start;
    task1.period = 100;
    task1.elapsedTime = task1.period;
    task1.TickFct = &player;

    // enemy
    task2.state = start;
    task2.period = 100;
    task2.elapsedTime = task2.period;
    task2.TickFct = &enemy;

    // ball
    task3.state = start;
    task3.period = 100;
    task3.elapsedTime = task3.period;
    task3.TickFct = &ball;

    // display
    task4.state = start;
    task4.period = 1;
    task4.elapsedTime = task4.period;
    task4.TickFct = &game;

    unsigned short i;
    A2D_init();
    Set_A2D_Pin(1);

    unsigned long GCD = tasks[0]->period;
    for(i = 0; i < numTasks; i++) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    TimerSet(GCD);
    TimerOn();
    while (1) {
        for(i = 0; i < numTasks; i++) {
            if(tasks[i]->elapsedTime == tasks[i]->period) {
                tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
                tasks[i]->elapsedTime = 0;
            }
            tasks[i]->elapsedTime += GCD;
        }
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
