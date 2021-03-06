/* 
* MotorController.cpp
*
* Created: 19-Mar-18 15:24:44
* Author: Robin C. Pel
*/

#include "MotorController.h"

 Controllers::MotorController::MotorController(Gpio::Pin* motorMultiplexPin, TC motorTimerCounter, Gpio::Pin motorTcPin, Gpio::Pin* rotationSensorPins):
    timerCounter(motorTimerCounter), 
    tcPin(motorTcPin)
{
    for (int i = 0; i < 3; i++)
    {
        multiplexPin[i] = motorMultiplexPin[i];
        Gpio::SetPinDirection(multiplexPin[i], Gpio::Dir::Output);
    }
    
    rotationSensor = new RotationSensor(rotationSensorPins);
    
    TimerCounter::SetClock(timerCounter, ClockValue::Div1);                             // Start the clock for the timercounter and set the prescaler to 1
    TimerCounter::SetWaveformGenMode(timerCounter, WaveformGenMode::SingleSlope);       // Set the waveform generation mode to Single slope PWM
    TimerCounter::EnableOnPin(timerCounter, Gpio::GetPinNumber(tcPin));                 // Enable the TC signal on the tc pin
    TimerCounter::SetPeriod(timerCounter, 532);                                         // Set the period to 532 (Source Clock / (Prescaler * (Period + 1)) = 60.037)
}

void Controllers::MotorController::rotateMotor(uint8_t motorNumber)
{
    setMuxChannel(motorNumberToMuxChannel[motorNumber]);
    
    TimerCounter::SetDutyCycleOnPin(timerCounter, 50, Gpio::GetPinNumber(tcPin));       // Set the duty cycle on pin 1 to 50%
    _delay_ms(400);                                                                     // Wait a little bit
    
    waitForRotation(motorNumber);                                                       // Wait until a gap is seen by the IR sensor
    TimerCounter::SetDutyCycleOnPin(timerCounter, 0, Gpio::GetPinNumber(tcPin));        // Set the duty cycle on pin 0 to 00%
}

void Controllers::MotorController::setMuxChannel(uint8_t muxChannel)
{
    for (int i = 0; i < 3; i++)
        Gpio::SetPinValue(multiplexPin[i], (Gpio::Value) (( muxChannel >> i ) & 0b1));
    
    _delay_ms(1);       // Just to be sure the multiplexer has switched its channel we'll wait for one millisecond
}

void Controllers::MotorController::waitForRotation(uint8_t motorNumber) 
{
    while (1) 
    {
        if (rotationSensor->getData() & (1 << motorNumber)) return;
    }
}
