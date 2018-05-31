/*
 * main.cpp
 *
 * Created: 06-Mar-18 11:56:46
 * Author: Robin C. Pel
 */


// Includes
#include "includes.h"
#include "Hardware/SystemClock.h"
#include "Hardware/Gpio.h"
#include "Hardware/GenericTC.h"
#include "Master/RaspberryPi.h"
#include "Controllers/CoolingController.h"
#include "Controllers/MotorController.h"
#include "Sensors/DistanceSensor.h"


// Name-spaces
using namespace Hardware;
using namespace Communication;
using namespace Master;
using namespace Controllers;
using namespace Gpio;
using namespace Sensors;
using namespace TimerCounter;


// LEDs
Pin greenLed = Pin::F3;
Pin yellowLed = Pin::F4;
Pin redLed = Pin::F6;
void runningLed( uint8_t ledVal ) { SetPinValue(greenLed, (Value) ledVal); }
void busyLed( uint8_t ledVal ) { SetPinValue(yellowLed, (Value) ledVal); }
void errorLed( uint8_t ledVal ) { SetPinValue(redLed, (Value) ledVal); }


// Raspberry Pi
RaspberryPi* raspberryPi;
Usart::RxTx raspberrySerialPort = Usart::RxTx::D2_D3;


// Cooling Controller Variables
CoolingController* coolingController;
Pin temperatureSensorPins[3] = { Pin::D6, Pin::D5, Pin::D4 };
Pin fanGroupPins[2] = { Pin::C0, Pin::C1 };
TC coolingTimerCounter = TimerCounter::TC::TC0C;


// Motor Controller Variables
MotorController* motorController;
Pin rotationSensorPins[8] = { Pin::A0, Pin::A1, Pin::B2, Pin::B3, Pin::B4, Pin::B5, Pin::B6, Pin::B7 };
TC motorTimerCounter = TimerCounter::TC::TC0D;
Pin motorMultiplexPins[3] = { Pin::F2, Pin::F1, Pin::F0 };


// Distance Sensor
DistanceSensor* distanceSensor;
Pin triggerPins[2] = { Pin::C3, Pin::C2 };
Pin echoPin = Pin::D0;
Pin distanceMultiplexPins[4] = { Pin::C7, Pin::C6, Pin::C5, Pin::C4 };
float emptyDistance = 100.0;


// Miscellaneous Variables
uint8_t const IDENTIFIER[] = { 0xAB, 0xBC, 0xCD, 0xDA };
bool locked = false;


// Functions
void executeLockCommand(uint8_t* response)
{
    if (locked)
    {
        errorLed(1);
        response[0] = (uint8_t) RaspberryPi::ComException::Locked;
        response[1] = 0x00;
    }
    else
    {
        locked = true;
        response[0] = (uint8_t) raspberryPi->getEquivalentCommandResponse(RaspberryPi::Command::Lock);
        response[1] = 0x00;
    }
}

void executeUnlockCommand(uint8_t* response)
{
    locked = false;
    response[0] = (uint8_t) raspberryPi->getEquivalentCommandResponse(RaspberryPi::Command::Unlock);
    response[1] = 0x00;
}

void executeSenseCommand(uint8_t* response)
{
    if (locked)
    {
        errorLed(1);
        response[0] = (uint8_t) RaspberryPi::ComException::Locked;
        response[1] = 0x00;
    }
    else
    {
        response[0] = (uint8_t) raspberryPi->getEquivalentCommandResponse(RaspberryPi::Command::Sense);
        response[1] = 0x04;
        
        response[2] = IDENTIFIER[0];
        response[3] = IDENTIFIER[1];
        response[4] = IDENTIFIER[2];
        response[5] = IDENTIFIER[3];
    }
}

void executeTemperatureCheckCommand(uint8_t* response, uint8_t* receivedCommand)
{
    if (locked)
    {
        errorLed(1);
        response[0] = (uint8_t) RaspberryPi::ComException::Locked;
        response[1] = 0x00;
    }
    else
    {
        response[0] = (uint8_t) raspberryPi->getEquivalentCommandResponse(RaspberryPi::Command::TemperatureCheck);
        response[1] = 0x00;
        
        if (receivedCommand[1] == 1)                // Target temperature has been supplied
        {
            // TODO: Execute a cooling configuration update
        } 
        else                                        // Target temperature has not been supplied
        {
            // TODO: Execute a cooling configuration update
        }
    }
}

void executeDispenseCommand(uint8_t* response, uint8_t* receivedCommand)
{
    if (locked)
    {
        errorLed(1);
        response[0] = (uint8_t) RaspberryPi::ComException::Locked;
        response[1] = 0x00;
    }
    else
    {
        if (receivedCommand[2] > 7) 
        {
            errorLed(1);
            response[0] = (uint8_t) RaspberryPi::ComException::Parameter;   // Add the "Not enough parameters" Exception as command response
            response[1] = 0x00;                                             // Zero parameters
        }
        else
        {
            response[0] = (uint8_t) raspberryPi->getEquivalentCommandResponse(RaspberryPi::Command::Dispense);
            response[1] = 0x01;
            
            motorController->rotateMotor(receivedCommand[2]);
            
            // TODO: Add whether the column just ran out of bottles or not
            if ( distanceSensor->getSimpleData() & (1 << receivedCommand[2]) ) 
                response[2] = 0x01;
            else
                response[2] = 0x00;
        }
    }
}

void executeDistanceCommand(uint8_t* response)
{
    if (locked)
    {
        response[0] = (uint8_t) RaspberryPi::ComException::Locked;
        response[1] = 0x00;
    }
    else 
    {
        response[0] = (uint8_t) raspberryPi->getEquivalentCommandResponse(RaspberryPi::Command::Distance);
        response[1] = 0x01;
        
        // TODO: Add the empty status of all columns
        response[2] = 0x00;
        
    }
}

void executecommand(uint8_t* response, uint8_t* receivedCommand)
{
    switch ((RaspberryPi::Command) receivedCommand[0])
    {
        case RaspberryPi::Command::Lock:                executeLockCommand(response);                                   break;
        case RaspberryPi::Command::Unlock:              executeUnlockCommand(response);                                 break;
        case RaspberryPi::Command::Sense:               executeSenseCommand(response);                                  break;
        case RaspberryPi::Command::TemperatureCheck:    executeTemperatureCheckCommand(response, receivedCommand);      break;
        case RaspberryPi::Command::Dispense:            executeDispenseCommand(response, receivedCommand);              break;
        case RaspberryPi::Command::Distance:            executeDistanceCommand(response);                               break;
        default:                                        /* Impossible */                                                break;
    }
}

void routine(void)
{
    while (1)
    {
        uint8_t operationStatus = raspberryPi->waitForNextCommand();        // 0 = success, 1 = command does not exist, 2 = timeout
        uint8_t* receivedCommand = raspberryPi->getCommand();               // Get the location to the received command
        uint8_t response[6] = { 0 };
        
        busyLed(1);
        
        switch (operationStatus)
        {
            case 0:     // Everything went fine
            executecommand(response, receivedCommand);
            break;
            
            
            case 1:     // Command does not exist
            errorLed(1);
            response[0] = (uint8_t) RaspberryPi::ComException::Unknown;     // Add the "Command Unknown" Exception as command response
            response[1] = 0x00;                                             // Zero parameters
            break;
            
            
            case 2:     // Timeout
            errorLed(1);
            response[0] = (uint8_t) RaspberryPi::ComException::TimeOut;     // Add the "Serial timeout" Exception as command response
            response[1] = 0x00;                                             // Zero parameters
            break;
        }
        
        raspberryPi->returnResponse(response);                              // Return the response
        
        busyLed(0);
        errorLed(0);
    }
}

void initialize(void)
{
    // Initialize the system clock and the generic timer-counter
    SystemClock::SetClockSource(SystemClock::Source::RC32MHz);
    TimerCounter::InitializeGenericTC();
    
    _delay_ms(1000);
    
    // Initialize the status LEDs
    SetPinDirection(greenLed, Dir::Output);
    SetPinDirection(yellowLed, Dir::Output);
    SetPinDirection(redLed, Dir::Output);
    
    
    // Initialize all the other hardware
    raspberryPi = new RaspberryPi(raspberrySerialPort);
    coolingController = new CoolingController(temperatureSensorPins, fanGroupPins);
    motorController = new MotorController(rotationSensorPins, motorTimerCounter, motorMultiplexPins);
    distanceSensor = new DistanceSensor(triggerPins, echoPin, distanceMultiplexPins, emptyDistance);
}

int main()
{
    initialize();
    runningLed(1);
    //routine();
    //runningLed(0);
    
    while (1) 
    {
        for (int i = 0; i < 8; i++) 
        {
            busyLed(1);
            motorController->rotateMotor(i);
            busyLed(0);
            _delay_ms(2000);
        }
        
    }
}
