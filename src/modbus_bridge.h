
/*
 This is simple bridge RTU <> RTU between two hardware serials
 it works for Foxess T10 G3 and Chint DTSU666
 it is based on example as below details

 this version is only : 
 - working bridge,
 - send serial display of some energy data from registers (energy, current, power, voltages)

 Next need to do:
 - add ESPHome as custom component integration to Homeassistant

based on GREAT library and examples from :
  Modbus ESP8266/ESP32
  Simple ModbesRTU  bridge
   
  (c)2020 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266
*/


#include <ModbusRTU.h>

#include "esphome.h"    // ESPHOME adding custom component

#define TO_REG 0x1000  //define chint modbus registers areas
#define TO_UABC_REG 0x2006
#define TO_BASE 0x0000
#define SLAVE_ID 1
#define PULL_ID 1
#define FROM_BASE 0x0000
#define FROM_REG 0x1000
#define FROM_UABC_REG 0x2006

class ModbusBridgeSerial : public Component, public Sensor
{
public:
	Sensor *PowerNow_sensor = new Sensor();
	Sensor *EnergyImportedTotal_sensor = new Sensor();
	Sensor *EnergyExportedTotal_sensor = new Sensor();
    Sensor *VoltageA_sensor = new Sensor();
    Sensor *VoltageB_sensor = new Sensor();
    Sensor *VoltageC_sensor = new Sensor();
	Sensor *CurrentA_sensor = new Sensor();
	Sensor *CurrentB_sensor = new Sensor();
	Sensor *CurrentC_sensor = new Sensor();
	Sensor *PowerA_sensor = new Sensor();
	Sensor *PowerB_sensor = new Sensor();
	Sensor *PowerC_sensor = new Sensor();

ModbusRTU mb1;
ModbusRTU mb2;

void setup() override {

  Serial2.begin(9600, SERIAL_8N1, 33, 32); //Init Serial2   //for modbus use serial 1 and 2 
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
  mb2.addHreg(TO_BASE);
}
unsigned long previousMillis = 0; //for printing debug sometimes
const long interval = 2000;  //every xx ms

float UA; //Voltages UABC
float UB;
float UC;
float IA; //Current I ABC
float IB;
float IC;
float PA; //Current P ABC
float PB;
float PC;
float Pnow;  //Power exporting(negative sign) or importing=consuming(positive) kW NOW
//float Pimp; //Power importing kW NOW
float EexpTot; //Total exported Energy kWh
float EimpTot;  //Total imported Energy kWh
int Reg = 1;  //pointer of last apdated register area
int Publish = 1;  //pointer for publishing sensors
bool newData = 0; //received new data from chint
uint16_t A_0 =0; //two regs for float but seems to be not neccesary
uint16_t A_1 =0;
float f_2uint_float(unsigned int uint1, unsigned int uint2) {    // reconstruct the float from 2 unsigned integers
  union f_2uint {
    float f;
    uint16_t i[2];
  };
  union f_2uint f_number;
  f_number.i[0] = uint1;
  f_number.i[1] = uint2;
  return f_number.f;
}
void loop() override {
  unsigned long currentMillis = millis();
  if(!mb1.slave()){
    newData = 1;
    switch(Reg) {
      case 1:
        mb1.pullHreg(1, FROM_REG, TO_REG,60);
        //Serial.println("pulling REG");
        Reg = 2;
      break;
      case 2:
        mb1.pullHreg(1, FROM_UABC_REG, TO_UABC_REG,40);
       // Serial.println("pulling UABC");
        Reg = 3;
      break;
      case 3:
        mb1.pullHreg(1, FROM_BASE, TO_BASE,10);
        //Serial.println("pulling BASE");
        Reg = 1;
      break;
    }
  }  
  if (newData){
    newData =0;
        switch(Reg) {
      case 1:
      break;
      case 2:
        //Serial.println("calculating UABC"); //TO_UABC_REG 0x2006
        UA = f_2uint_float(mb1.Hreg(0x2007),mb1.Hreg(0x2006))*0.1; //converting registers uint16 to signed float BIG endian
        IA = f_2uint_float(mb1.Hreg(0x200D),mb1.Hreg(0x200C))*0.001; // all as above
        PA = f_2uint_float(mb1.Hreg(0x2015),mb1.Hreg(0x2014))*0.1;
        UB = f_2uint_float(mb1.Hreg(0x2009),mb1.Hreg(0x2008))*0.1;
        IB = f_2uint_float(mb1.Hreg(0x200F),mb1.Hreg(0x200E))*0.001;
        PB = f_2uint_float(mb1.Hreg(0x2017),mb1.Hreg(0x2016))*0.1;
        UC = f_2uint_float(mb1.Hreg(0x200B),mb1.Hreg(0x200A))*0.1;
        IC = f_2uint_float(mb1.Hreg(0x2011),mb1.Hreg(0x2010))*0.001;
        PC = f_2uint_float(mb1.Hreg(0x2019),mb1.Hreg(0x2018))*0.1;
        Pnow = f_2uint_float(mb1.Hreg(0x2013),mb1.Hreg(0x2012))*0.1; //2012H Pt Combined active power, Unit W(×0.1W) Power exporting(negative sign) or importing=consuming(positive) kW NOW
      break;
      case 3:  
        //Serial.println("calculating REG"); //TO_REG 0x1000
        EexpTot = f_2uint_float(mb1.Hreg(0x1029),mb1.Hreg(0x1028));    //1028H ExpEp (current)negative total active energy(kWh) oddane kilowaty suma
        EimpTot = f_2uint_float(mb1.Hreg(0x101f),mb1.Hreg(0x101e));    //101eH total imported (consumed) energy kWh
      break;
    }
  }
  mb1.task();  //work time call for modbus lib
  mb2.task();
  if (currentMillis - previousMillis >= interval) { //time to print some debug
    previousMillis = currentMillis;
 /*
	Serial.print(" UA="); 
    Serial.print(UA);
    Serial.print(" UB=");
    Serial.print(UB);
    Serial.print(" UC=");
    Serial.print(UC);
    Serial.print(" IA=");
    Serial.print(IA);
    Serial.print(" IB=");
    Serial.print(IB);
    Serial.print(" IC=");
    Serial.print(IC);
    Serial.print(" PA=");
    Serial.print(PA);
    Serial.print(" PB=");
    Serial.print(PB);
    Serial.print(" PC=");
    Serial.println(PC);
    Serial.print(" Pnow=");
    Serial.print(Pnow);
    Serial.print(" EimpTot=");
    Serial.print(EimpTot);
    Serial.print(" EexpTot=");
    Serial.println(EexpTot);
	*/

    switch(Publish) {
      case 1:
			PowerNow_sensor->publish_state(Pnow);
			EnergyImportedTotal_sensor->publish_state(EimpTot);
			EnergyExportedTotal_sensor->publish_state(EexpTot);
      break;
	  case 2:
			VoltageA_sensor->publish_state(UA);
			VoltageB_sensor->publish_state(UB);
			VoltageC_sensor->publish_state(UC);
      break;
      case 3:
			CurrentA_sensor->publish_state(IA);
			CurrentB_sensor->publish_state(IB);
			CurrentC_sensor->publish_state(IC);
      break;
	  case 4:
			PowerA_sensor->publish_state(PA);
			PowerB_sensor->publish_state(PB);
			PowerC_sensor->publish_state(PC);
      break;	  
	}

	Publish = Publish + 1;
	if(Publish== 5) {
		Publish = 1;
	}
	
  }
 
}
};
