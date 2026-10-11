# FSM_StepperMotor  — TM4C123GH6PM

A simple program that allows a user to control the 28BYJ-48 stepper motor.

The stepper motor is controled by the ULN2003 driver which wired to the MCU. The
stepper motor draws current which the MCU is not able to provide and thus must
be powered by a separate 5v power supply.

The program uses a Finite State Machine (FSM) architecture and the PLL
(80 MHz) with SysTick for accurate timing.

## How it works
- Press and hold the SW1 to spin the motor clockwise. 
- Press and hold the SW2 to spin the motor counterclockwise.
- The motor is idle when the switches not pressed or pressed simultaneously.


## Environment
Project is build with Code Composer Studio(Theia) using GNU Arm Toolchain and CMSIS.

## Hardware
- TM4C123 LaunchPad
- 28BYJ-48 stepper motor
- ULN2003 driver 
- Jumper wires
- 5v power supply

## Requirements
- Code Composer Studio (CCS) installed
- GNU Arm Toolchain installed and added to the CCS

## Usage
1. Wire the stepper motor to the driver, wire the driver to the MCU and 5v power supply.
2. Open the FSM_StepperMotor project in CCS.
3. Flash the code to the board.
4. Hold SW1 and/or SW2 to rotate the stepper motor.









