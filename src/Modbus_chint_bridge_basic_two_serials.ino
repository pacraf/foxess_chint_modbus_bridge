/*
 This is simple bridge RTU <> RTU between two hardware serials
 it works for Foxess T10 G3 and Chint DTSU666
 it is based on exmaple as below details

 this version is only proof of working bridge, does nothing additinally
 Next need to add:
 extraction of parameters from registers (energy, current, power, voltages)
 add ESPHome as custom component integration to Homeassistant


  Modbus ESP8266/ESP32
  Simple ModbesRTU  bridge
   
  (c)2020 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266
*/

#include <ModbusIP_ESP8266.h>
#include <ModbusRTU.h>

#define TO_REG 0x1000
#define TO_UABC_REG 0x2006
#define TO_BASE 0x0000
#define SLAVE_ID 1
#define PULL_ID 1
#define FROM_BASE 0x0000
#define FROM_REG 0x1000
#define FROM_UABC_REG 0x2006


ModbusRTU mb1;
ModbusRTU mb2;

void setup() {

 
//  

  Serial2.begin(9600, SERIAL_8N1, 33, 32); //Init Serial2  
  Serial1.begin(9600, SERIAL_8N1, 19, 18); // Init Serial1 
  Serial.begin(115200, SERIAL_8N1); // default pins for ESP32 debug

  mb2.begin(&Serial2, 27); // slave = emu chint
  mb1.begin(&Serial1, 17);  // master for chint on Serial1

  Serial.println("setup");
  mb1.master();   //master for  chint
  mb2.server(); //server = slave = emulator of chint
  mb2.slave(SLAVE_ID);
  mb2.addHreg(TO_REG);
  mb2.addHreg(TO_UABC_REG);
}

void loop() {
  if(!mb1.slave())
    mb1.pullHreg(1, FROM_REG, TO_REG,90);
  mb1.task();
  mb2.task();
  yield();
  delay(10); 
  Serial.println("loop");
  if(!mb1.slave())
    mb1.pullHreg(1, FROM_UABC_REG, TO_UABC_REG,40);
  mb1.task();
  mb2.task();
  delay(10);
  if(!mb1.slave())
    mb1.pullHreg(1, FROM_BASE, TO_BASE,10);
  mb1.task();
  mb2.task();
  yield();
  delay(10); 
}