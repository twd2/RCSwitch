/*
    RCSwitch - Arduino libary for remote control outlet switches
    Copyright (c) 2011 Suat Özgür.  All right reserved.

    Contributors:
    - Andre Koehler / info(at)tomate-online(dot)de
    - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
    - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=48

    Project home: http://code.google.com/p/rc-switch/

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


    Changes
        - prevent multiple creation of ISR. 31st May 2012 JP Liew

*/

#include "RCSwitch.h"

unsigned long long RCSwitch::nReceivedValue = 0;
unsigned int RCSwitch::nReceivedBitlength = 0;
unsigned int RCSwitch::nReceivedDelay = 0;
unsigned int RCSwitch::nReceivedProtocol = 0;
unsigned long RCSwitch::timings[RCSWITCH_MAX_CHANGES];
int RCSwitch::nReceiveTolerance = 60;

RCSwitch::RCSwitch()
{
    this->nReceiverInterrupt = -1;
    this->nTransmitterPin = -1;
    RCSwitch::nReceivedValue = 0;
    this->setPulseLength(350);
    this->setRepeatTransmit(10);
    this->setReceiveTolerance(60);
    this->setProtocol(1);
}

/**
  * Sets the protocol to send.
  */
void RCSwitch::setProtocol(int nProtocol)
{
    this->nProtocol = nProtocol;
    if (nProtocol == 1)
    {
        this->setPulseLength(350);
    }
    else if (nProtocol == 2) {
        this->setPulseLength(650);
    }
}

/**
  * Sets the protocol to send with pulse length in microseconds.
  */
void RCSwitch::setProtocol(int nProtocol, int nPulseLength)
{
    this->nProtocol = nProtocol;
    if (nProtocol == 1) {
        this->setPulseLength(nPulseLength);
    }
    else if (nProtocol == 2) {
        this->setPulseLength(nPulseLength);
    }
}

/**
  * Sets pulse length in microseconds
  */
void RCSwitch::setPulseLength(int nPulseLength) 
{
    this->nPulseLength = nPulseLength;
}

/**
 * Sets Repeat Transmits
 */
void RCSwitch::setRepeatTransmit(int nRepeatTransmit)
{
    this->nRepeatTransmit = nRepeatTransmit;
}

/**
 * Set Receiving Tolerance
 */
void RCSwitch::setReceiveTolerance(int nPercent)
{
    RCSwitch::nReceiveTolerance = nPercent;
}

/**
 * Enable transmissions
 *
 * @param nTransmitterPin    Arduino Pin to which the sender is connected to
 */
void RCSwitch::enableTransmit(int nTransmitterPin)
{
    this->nTransmitterPin = nTransmitterPin;
    pinMode(this->nTransmitterPin, OUTPUT);
}

/**
  * Disable transmissions
  */
void RCSwitch::disableTransmit()
{
    pinMode(this->nTransmitterPin, INPUT);
    this->nTransmitterPin = -1;
}

/**
 * Sends a Code Word
 * @param sCodeWord   /^[10FS]*$/  -> see getCodeWord
 */
void RCSwitch::sendTriState(char* sCodeWord) 
{
    for (int nRepeat=0; nRepeat < nRepeatTransmit; nRepeat++)
    {
        int i = 0;
        while (sCodeWord[i] != '\0') 
        {
            switch(sCodeWord[i])
            {
                case '0':
                    this->sendT0();
                    break;
                case 'F':
                    this->sendTF();
                    break;
                case '1':
                    this->sendT1();
                    break;
            }
            i++;
        }
        this->sendSync();
    }
}

void RCSwitch::send(char* sCodeWord)
{
    for (int nRepeat=0; nRepeat < nRepeatTransmit; nRepeat++)
    {
        int i = 0;
        while (sCodeWord[i] != '\0') 
        {
            switch(sCodeWord[i])
            {
                case '0':
                    this->send0();
                    break;
                case '1':
                    this->send1();
                    break;
            }
            i++;
        }
        this->sendSync();
    }
}

void RCSwitch::transmit(int nHighPulses, int nLowPulses)
{
    boolean disabled_Receive = false;
    int nReceiverInterrupt_backup = nReceiverInterrupt;
    if (this->nTransmitterPin != -1)
    {
        if (this->nReceiverInterrupt != -1)
        {
            this->disableReceive();
            disabled_Receive = true;
        }
        digitalWrite(this->nTransmitterPin, HIGH);
        delayMicroseconds(this->nPulseLength * nHighPulses);
        digitalWrite(this->nTransmitterPin, LOW);
        delayMicroseconds(this->nPulseLength * nLowPulses);
        if(disabled_Receive)
        {
            this->enableReceive(nReceiverInterrupt_backup);
        }
    }
}

/**
 * Sends a "0" Bit
 *                       _    
 * Waveform Protocol 1: | |___
 *                       _  
 * Waveform Protocol 2: | |__
 */
void RCSwitch::send0()
{
    if (this->nProtocol == 1)
    {
        this->transmit(1,3);
    }
    else if (this->nProtocol == 2)
    {
        this->transmit(1,2);
    }
}

/**
 * Sends a "1" Bit
 *                       ___  
 * Waveform Protocol 1: |   |_
 *                       __  
 * Waveform Protocol 2: |  |_
 */
void RCSwitch::send1()
{
    if (this->nProtocol == 1)
    {
        this->transmit(3,1);
    }
    else if (this->nProtocol == 2)
    {
        this->transmit(2,1);
    }
}

/**
 * Sends a Tri-State "0" Bit
 *            _     _
 * Waveform: | |___| |___
 */
void RCSwitch::sendT0()
{
    this->transmit(1,3);
    this->transmit(1,3);
}

/**
 * Sends a Tri-State "1" Bit
 *            ___   ___
 * Waveform: |   |_|   |_
 */
void RCSwitch::sendT1()
{
    this->transmit(3,1);
    this->transmit(3,1);
}

/**
 * Sends a Tri-State "F" Bit
 *            _     ___
 * Waveform: | |___|   |_
 */
void RCSwitch::sendTF()
{
    this->transmit(1,3);
    this->transmit(3,1);
}

/**
 * Sends a "Sync" Bit
 *                       _
 * Waveform Protocol 1: | |_______________________________
 *                       _
 * Waveform Protocol 2: | |__________
 */
void RCSwitch::sendSync()
{
    if (this->nProtocol == 1)
    {
        this->transmit(1,31);
    }
    else if (this->nProtocol == 2)
    {
        this->transmit(1,10);
    }
}

/**
 * Enable receiving data
 */
void RCSwitch::enableReceive(int interrupt)
{
    if (this->nReceiverInterrupt == interrupt) return; // prevent multiple creation of ISR. 31st May 2012 JP Liew
    this->nReceiverInterrupt = interrupt;
    this->enableReceive();
}

void RCSwitch::enableReceive()
{
    if (this->nReceiverInterrupt != -1)
    {
        RCSwitch::nReceivedValue = NULL;
        RCSwitch::nReceivedBitlength = NULL;
        attachInterrupt(this->nReceiverInterrupt, handleInterrupt, CHANGE);
    }
}

/**
 * Disable receiving data
 */
void RCSwitch::disableReceive()
{
    detachInterrupt(this->nReceiverInterrupt);
    this->nReceiverInterrupt = -1;
}

bool RCSwitch::available()
{
    return RCSwitch::nReceivedValue != NULL;
}

void RCSwitch::resetAvailable()
{
    RCSwitch::nReceivedValue = NULL;
}

unsigned long long RCSwitch::getReceivedValue()
{
    return RCSwitch::nReceivedValue;
}

unsigned int RCSwitch::getReceivedBitlength()
{
    return RCSwitch::nReceivedBitlength;
}

unsigned int RCSwitch::getReceivedDelay()
{
    return RCSwitch::nReceivedDelay;
}

unsigned int RCSwitch::getReceivedProtocol()
{
    return RCSwitch::nReceivedProtocol;
}

unsigned long* RCSwitch::getReceivedRawdata()
{
    return RCSwitch::timings;
}

static inline unsigned long diff(long A, long B)
{
  return abs(A - B);
}

bool RCSwitch::receiveProtocol1(unsigned int changeCount)
{
    unsigned long code = 0;
    unsigned long period = RCSwitch::timings[0] / 31;
    unsigned long tolerance = period * RCSwitch::nReceiveTolerance / 100;

    for (unsigned int i = 1; i < changeCount; i += 2)
    {
        code <<= 1;
        if (diff(RCSwitch::timings[i], period) < tolerance &&
            diff(RCSwitch::timings[i + 1], period * 3) < tolerance)
        {
            code |= 0;
        }
        else if (diff(RCSwitch::timings[i], period * 3) < tolerance &&
                 diff(RCSwitch::timings[i + 1], period) < tolerance)
        {
            code |= 1;
        }
        else
        {
            // Failed
            code = 0;
            break;
        }
    }

    if (changeCount > 47)
    {
        if (code != 0)
        {
            RCSwitch::nReceivedValue = code;
            RCSwitch::nReceivedBitlength = changeCount / 2;
            RCSwitch::nReceivedDelay = period;
            RCSwitch::nReceivedProtocol = 1;
        }
    }

    if (code == 0)
        return false;
    else
        return true;
}

void RCSwitch::handleInterrupt()
{
    static unsigned int changeCount;
    static unsigned long lastTime;
    static unsigned int repeatCount;

    long time = micros();
    unsigned long duration = time - lastTime;

    if ((duration >= 100000L && RCSwitch::timings[0] >= 100000L) || // T=4ms
        (duration > 5000 && RCSwitch::timings[0] > 5000 && RCSwitch::timings[0] < 100000L)) // normal
    {
        ++repeatCount;
        if (repeatCount == 1)
        {
            if (changeCount > 20)
            {
                --changeCount;
                receiveProtocol1(changeCount);
            }
            repeatCount = 0;
        }
        changeCount = 0;
    }
    else if ((duration > 5000 && RCSwitch::timings[0] <= 5000) || (duration >= 100000L))
    {
        changeCount = 0;
        repeatCount = 0;
    }

    if (changeCount >= RCSWITCH_MAX_CHANGES)
    {
        changeCount = 0;
        repeatCount = 0;
    }
    RCSwitch::timings[changeCount] = duration;
    ++changeCount;
    lastTime = time;
}
