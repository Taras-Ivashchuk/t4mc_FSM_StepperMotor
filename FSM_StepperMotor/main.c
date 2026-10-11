/*
Project: Stepper Motor Controller (Finite State Machine)

Description:
Controls a 28BYJ-48 5V Stepper Motor to rotate in clockwise or counterclockwise
directions. The Stepper Motor must be powered by separate external supply for
5V.

Behavior:
To move the stepper motor in clockwise direction press and hold SW1. To move the
stepper motor in the counterclockwise direction press and hold SW2. If both
switches are realesed or if both switches are pressed and hold at the same time
the stepper motor doesn't move.

Clockwise:
Pin/Step
        1   2   3   4   5   6   7   8
-------------------------------------------------------------------
Blue    1   1   1   1   1   0   0   0
Pink    1   1   1   0   0   0   1   1
Yellow  1   0   0   0   1   1   1   1
Orange  0   0   1   1   1   1   1   0
--------------------------------------------------------------------
        14  12  13  9   11  3   7   6

Counterclockwise: 6 7 3 11 9 13 12 14 (the opposite direction)

State transition table
                SW1 SW2
State/Input 00  01  10  11
----------------------------
14          14  6   12  14
12          12  14  13  12
13          13  12  9   13
9           9   13  11  9
11          11  9   3   11
3           3   11  7   3
7           7   3   6   7
6           6   7   14  6

Outputs:
PC7-4

Inputs:
SW1 - PF4
SW2 - PF0

00 - 0   no switch pressed (no moving)
01 - 1   SW2 pressed (counterclockwise)
10 - 2   SW1 pressed (clockwise)
11 - 3   SW1 and SW2 pressed (no moving)

Clock and software:
- CMSIS: the program uses CMSIS header (TM4C123GH6PM.h)
- PLL: the 16 MHz crystal is used to run the system clock at 80 MHz.

Inputs (active low, buttons with internal pull-ups):
- SW1 (PF4) = move the stepper motor clockwise
- SW2 (PF0) = move the stepper motor counterclockwise

Outputs:
   PC7-4 wired to the UL2003 driver

Hardware:
- TM4C123 LaunchPad
- 28BYJ-48 5V Stepper Motor
- UL2003 driver
- Jumper wires
- Power supply 5V
*/

#include "TM4C123GH6PM.h"

#define SW1 (1 << 4U)                   // control clockwise direction
#define SW2 (1 << 0U)                   // control counterclockwise direction
#define T1 1                            // 5 ms
#define INPUT (GPIOF->DATA_Bits[0x11])  // PF0, PF4
#define OUTPUT (GPIOC->DATA_Bits[0xF0]) // PC7 - PC4

#define S14 (&FSM_Controller[0])
#define S12 (&FSM_Controller[1])
#define S13 (&FSM_Controller[2])
#define S9 (&FSM_Controller[3])
#define S11 (&FSM_Controller[4])
#define S3 (&FSM_Controller[5])
#define S7 (&FSM_Controller[6])
#define S6 (&FSM_Controller[7])

// prototypes
void initGPIOF(void);            // initialize the switches
void initGPIOC(void);            // initialize the stepper motor pins
void initPLL(void);              // initalize the PLL
void initSysTick(void);          // initialize the SysTick timer
void wait5ms(void);              // wait 5 ms
void waitIn5ms(long unsigned i); // wait in 5ms units

struct State {
    unsigned long out; // 4-bit pattern to output padded to 8 bits
    unsigned long
        delay; // delay in 5ms units, to let the stepper motor respond
    const struct State
        *nextStates[4]; // next states which correspond to inputs 00 01 10 11
};

const struct State FSM_Controller[8] = {
    {0xE, T1, {S14, S6, S12, S14}}, {0xC, T1, {S12, S14, S13, S12}},
    {0xD, T1, {S13, S12, S9, S13}}, {0x9, T1, {S9, S13, S11, S9}},
    {0xB, T1, {S11, S9, S3, S11}},  {0x3, T1, {S3, S11, S7, S3}},
    {0x7, T1, {S7, S3, S6, S7}},    {0x6, T1, {S6, S7, S14, S6}},

};

int main(void) {
    initPLL();
    initSysTick();
    initGPIOF();
    initGPIOC();

    const struct State *state = S14; //  go clockwise initially
    volatile unsigned long input;

    while (1) {
        OUTPUT = (state->out) << 4;
        waitIn5ms(state->delay);
        input = ~INPUT;
        input = (input & 0x01) | ((input & 0x10) >> 3); // 0, 1, 2, 3
        state = state->nextStates[input];
    }
    // return 0; // unreachable code
}

void initGPIOF(void) {
    // enable clocking for  GPIOF switches
    SYSCTL->RCGCGPIO |= (1 << 5U);

    // dummy read to add small delay before clocking activates
    volatile unsigned long delay = SYSCTL->RCGCGPIO;

    // unlock the PF0 register
    GPIOF->LOCK = 0x4C4F434B;
    GPIOF->CR |= SW2;

    // disable analog functions for switches
    GPIOF->AMSEL &= ~(SW1 | SW2);

    // enable direction for switches to input
    GPIOF->DIR &= ~(SW1 | SW2);

    // no alternate functions
    GPIOF->AFSEL &= ~(SW1 | SW2);
    GPIOF->PCTL &= ~(0x0F | (0x0F << 16));

    // internal pull-ups
    GPIOF->PUR |= (SW1 | SW2);

    // enable digital pins
    GPIOF->DEN |= (SW1 | SW2);
}

void initGPIOC(void) {
    // enable clocking for  GPIOC
    SYSCTL->RCGCGPIO |= (1U << 2);

    // dummy read to add small delay before clocking activates
    volatile unsigned long delay = SYSCTL->RCGCGPIO;

    // disable analog functions for PC7-4
    GPIOC->AMSEL &= ~(0x0F << 4);

    // enable output direction for PC7-4
    GPIOC->DIR |= (0x0F << 4);

    // no alternate functions for PC7-4
    GPIOC->AFSEL &= ~(0x0F << 4);
    GPIOC->PCTL &= ~0xFFFF0000;

    // enable digital pins PC7-4
    GPIOC->DEN |= (0x0F << 4);
}

void initPLL(void) {
    // Important: Write the RCC register prior to writing the RCC2 register.
    SYSCTL->RCC = (SYSCTL->RCC & ~0x000007C0) // clear XTAL field, bits 10-6
                  + 0x00000540; // 10101, configure for 16 MHz crystal
    SYSCTL->RCC2 |= 0x80000000; // USERRCC2, USE RCC2 to override RCC
    SYSCTL->RCC2 |= 0x00000800; // BYPASS2, PLL bypass

    SYSCTL->RCC2 &= ~0x00000070; // configure for main oscillator source
    SYSCTL->RCC2 &= ~0x00002000; // activate by clearing PWRDN bit
    // set the desired system divider
    SYSCTL->RCC2 |= 0x40000000; // use 400 MHz PLL by setting DIV400
    SYSCTL->RCC2 = (SYSCTL->RCC2 & ~0x1FC00000) // clear system clock divider
                   + (4 << 22); // configure for 80 MHz clock, 400 / 5
    // wait for the PLL to lock by polling PLLRIS
    while ((SYSCTL->RIS & 0x00000040) == 0) {
    }; // wait for PLLRIS bit indicating the PLL timer is ready
    SYSCTL->RCC2 &= ~0x00000800; // enable PLL by clearing the BYPASS2 bit
}

void initSysTick(void) {
    SysTick->CTRL = 0x0;        // disable Systick
    SysTick->VAL = 0x0;         // clear the counter
    SysTick->LOAD = 0x00FFFFFF; // set the maximum value
    SysTick->CTRL = 0x00000005; // source the Systick with core clock
}

void wait5ms(void) {
    //  one tick takes 1 / 80 MhZ = 12.5 ns
    //  ticks needed for 5 ms = 5 ms / 12.5 ns = 400 000
    SysTick->LOAD = 0x00061A80 - 1; // 400 000 - 1
    SysTick->VAL = 0x00;            // clear the counter
    while ((SysTick->CTRL & (1 << 16U)) ==
           0) { // setting the COUNT means the timer has counted to 0
        ;
    }
}

void waitIn5ms(long unsigned i) {
    while (i--)
        wait5ms();
}

// Needed for startup_TM4C123GH6PM.c
__attribute__((naked)) void assert_failed(char const *file, int line) {
    /* TBD: damage control */
    NVIC_SystemReset(); /* reset the system */
}
