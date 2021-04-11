/*
  loratx.ino - Main file for the project

  Copyright (C) 2021  Marius Bezuidenhout

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// include the library
#include <RadioLib.h>
#include <Arduino.h>
#include <functionlib.h>

#define RADIO_FREQ_MHZ 868

#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define RX 3
#define TX 1

enum RadioAction { RADIO_OFF, RADIO_TX, RADIO_RX };
enum TimeAwareActions { FUNC_MINUTE, FUNC_SECOND, FUNC_100_MSEC };

void ICACHE_RAM_ATTR setFlag(void);
void ICACHE_RAM_ATTR D2Read(void);

// RF95 has the following connections:
// CS pin:    15
// DIO0 pin:  5
// RESET pin: 16
// DIO1 pin:  3
RFM95 radio = new Module(15, 5, 16);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//RFM95 radio = RadioShield.ModuleA;

// save transmission state between loops
int transmissionState = ERR_NONE;
uint16_t radioAction = RADIO_OFF;
uint16_t debounce = 50;

struct INTERRUPTS {
  bool interruptAttached = false;
  bool pinState = LOW;
  bool stateTransmitted = false;
} inputInterrupts[3];

void setup() {
  Serial.begin(115200);

  // initialize RFM95 with default settings
  Serial.print(F("[RFM95] Initializing ... "));
  int state = radio.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  radio.setFrequency(RADIO_FREQ_MHZ);

  // NOTE: some RFM95 modules use high power output,
  //       those are usually marked RFM95(C/CW).
  //       To configure RadioLib for these modules,
  //       you must call setOutputPower() with
  //       second argument set to true.
  /*
    Serial.print(F("[RFM95] Setting high power module ... "));
    state = radio.setOutputPower(20, true);
    if (state == ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true);
    }
  */

  // start transmitting the first packet
  Serial.print(F("[RFM95] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 64 characters long

  // set the function that will be called
  // when packet transmission is finished
  radio.setDio0Action(setFlag);
  
  transmissionState = radio.startTransmit("System up");
  radioAction = RADIO_TX;

  // you can also transmit byte array up to 64 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    state = radio.startTransmit(byteArr, 8);
  */

  pinMode(D2, INPUT_PULLUP);
}

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  if(radioAction == RADIO_TX) {
    transmittedFlag = true;
  } else if(radioAction == RADIO_RX) {
    receivedFlag = true;    
  }
  radioAction = RADIO_OFF;
}

void checkLoraSend(void)
{
  if(transmittedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;
  
    // reset flag
    transmittedFlag = false;

    // Radio is now off
    radioAction = RADIO_OFF;
  
    if (transmissionState == ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));
  
      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()
  
    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }
    
    // we're ready for more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}

void checkLoraRecv(void)
{
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    if (state == ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[RFM95] Received packet!"));

      // print data of the packet
      Serial.print(F("[RFM95] Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      // of the last received packet
      Serial.print(F("[RFM95] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);

    }

    // we're ready for more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}

bool loraSend(String msg) {
  if ( radioAction == RADIO_TX ) { // Wait for transmit to complete
    return false;
  }
  // send keep alive
  Serial.print(F("[RFM95] Sending packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  radioAction = RADIO_TX;
  transmissionState = radio.startTransmit(msg);

  // you can also transmit byte array up to 64 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    int state = radio.startTransmit(byteArr, 8);
  */

  // enable interrupt service routine
  enableInterrupt = true;
  return true;
}

void sendPinState() {
  uint8_t i = 0;
  for ( i = 0; i < 3; i++ ) {
    if ( inputInterrupts[i].interruptAttached && !inputInterrupts[i].stateTransmitted && loraSend((String) "Pin" + i + "State" + inputInterrupts[i].pinState)) {
      inputInterrupts[i].stateTransmitted = true;
    }
  }
}

void timeLoop(uint8_t action)
{
  switch (action) {
    case FUNC_MINUTE:
      loraSend("SYN");
      break;    
    case FUNC_SECOND:
      break;
    case FUNC_100_MSEC:
      checkLoraSend();
      checkLoraRecv();
      sendPinState();
      break;
  }
}

void D2Read(void) {
  static bool lastState = LOW;
  static uint32_t lastDebounceTime = millis();

  bool pinState = digitalRead(D2);

  if (pinState != lastState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounce) {
    if (pinState != inputInterrupts[0].pinState) {
      inputInterrupts[0].pinState = pinState;
      inputInterrupts[0].stateTransmitted = false;
    }
  }
  lastState = pinState;
}

void loop() {
  static uint32_t state_minute = 0;                // State minute timer
  static uint32_t state_second = 0;                // State second timer
  static uint32_t state_100_msec = 0;              // State 100 msec timer
  if (radioAction == RADIO_OFF) {                   // We are not waiting for anything so lets listen for packets
    radioAction = RADIO_RX;
    radio.startReceive();
  }
  if (!inputInterrupts[0].interruptAttached) {
    inputInterrupts[0].interruptAttached = true;
    attachInterrupt(digitalPinToInterrupt(D2), D2Read, CHANGE);
  }
  
  if (TimeReached(state_minute)) {
    SetNextTimeInterval(state_minute, 60000);
    timeLoop(FUNC_MINUTE);
  }
  if (TimeReached(state_second)) {
    SetNextTimeInterval(state_second, 1000);
    timeLoop(FUNC_SECOND);
  }
  if (TimeReached(state_100_msec)) {
    SetNextTimeInterval(state_100_msec, 100);
    timeLoop(FUNC_100_MSEC);
  }
}