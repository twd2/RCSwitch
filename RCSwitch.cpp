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
void RCSwitch::setRepeatTransmit(unsigned int nRepeatTransmit)
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
 * @param nTransmitterPin    Arduino Pin to which the transmitter is connected to
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
 * @param sCodeWord   a tristate code word consisting of the letter 0, 1, F
 */
void RCSwitch::sendTriState(char* sCodeWord) 
{
    // turn the tristate code word into the corresponding bit pattern, then send it
    unsigned long code = 0;
    unsigned int length = 0;
    for (const char* p = sCodeWord; *p; p++)
    {
        code <<= 2L;
        switch (*p)
        {
        case '0':
            // bit pattern 00
            break;
        case 'F':
            // bit pattern 01
            code |= 0b01L;
            break;
        case '1':
            // bit pattern 11
            code |= 0b11L;
            break;
        }
        length += 2;
    }
    this->send(code, length);
}

void RCSwitch::send(char* sCodeWord)
{
    // turn the bistate code word into the corresponding bit pattern, then send it
    unsigned long code = 0;
    unsigned int length = 0;
    for (const char* p = sCodeWord; *p; p++)
    {
        code <<= 1L;
        if (*p != '0')
            code |= 1L;
        length++;
    }
    this->send(code, length);
}

void RCSwitch::send(unsigned long ulCode, unsigned int nLength)
{
    if (this->nTransmitterPin == -1)
        return;

    boolean disabledReceive = false;
    int nReceiverInterrupt_backup = nReceiverInterrupt;
    if (this->nReceiverInterrupt != -1)
    {
        this->disableReceive();
        disabledReceive = true;
    }

    for (unsigned int nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++)
    {
        for (unsigned int i = 0; i < nLength; ++i)
        {
            if (((ulCode >> i) & 1) == 0)
            {
                this->send0();
            }
            else
            {
                this->send1();
            }
        }
        this->sendSync();
    }

    if(disabledReceive)
    {
        this->enableReceive(nReceiverInterrupt_backup);
    }
}

void RCSwitch::transmit(int nHighPulses, int nLowPulses)
{
    digitalWrite(this->nTransmitterPin, HIGH);
    delayMicroseconds(this->nPulseLength * nHighPulses);
    digitalWrite(this->nTransmitterPin, LOW);
    delayMicroseconds(this->nPulseLength * nLowPulses);
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
        this->transmit(1, 3);
    }
    else if (this->nProtocol == 2)
    {
        this->transmit(1, 2);
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
        this->transmit(3, 1);
    }
    else if (this->nProtocol == 2)
    {
        this->transmit(2, 1);
    }
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
        this->transmit(1, 31);
    }
    else if (this->nProtocol == 2)
    {
        this->transmit(1, 10);
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
        RCSwitch::nReceivedValue = 0;
        RCSwitch::nReceivedBitlength = 0;
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
    return RCSwitch::nReceivedValue != 0;
}

void RCSwitch::resetAvailable()
{
    RCSwitch::nReceivedValue = 0;
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
