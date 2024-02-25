//---------------------------------------------------------------------------
////////////////////////////////// LIBRERIE /////////////////////////////////
//---------------------------------------------------------------------------

#include <RS485_Wind_Direction_Transmitter_V2.h>
#include <AltSoftSerial.h>
#include <DFRobot_SHT3x.h>
#include <Wire.h>

//---------------------------------------------------------------------------
//////////////////////////////// DEFINES e VARIABILI ////////////////////////
//---------------------------------------------------------------------------

//Pluviometro
int sensorValue;
int goccia = 0;
float Pioggia = 0;
bool copie = false;
bool conteggio = false;

//Timer acquisizione dati
unsigned long t1,tp,ts,dt,dtp,dts;
bool ConteggioSensori = false;

//Anemometro Intensità
float Inensita = 0;
float Level = 0;
float I = 0;
int sensor2Value;
float outvoltage =0;

//Termometro
DFRobot_SHT3x sht3x(&Wire,/*address=*/0x44,/*RST=*/6);
float Temperatura = 0;
float T = 0;
float Tvalue = 0;
float Umidita = 0;
float U = 0;
float Uvalue = 0;
int media = 0;

//Anemometro
#if defined(ARDUINO_AVR_UNO)||defined(ESP8266)   // use soft serial port
SoftwareSerial softSerial(/*rx =*/2, /*tx =*/3);
RS485_Wind_Direction_Transmitter_V2 windDirection(/*softSerial =*/&softSerial);
#elif defined(ESP32)   // use the hard serial port with remapping pin : Serial1
RS485_Wind_Direction_Transmitter_V2 windDirection(/*hardSerial =*/&Serial1, /*rx =*/D3, /*tx =*/D2);
#else   // use the hard serial port : Serial1
RS485_Wind_Direction_Transmitter_V2 windDirection(/*hardSerial =*/&Serial1);
#endif

//Anemometro direzione
int Direction = 0;
float Angle = 0;
uint8_t Address = 0x03;

const char* Orientation[17] = {
  "North", "North-northeast", "Northeast", "East-northeast", "East", "East-southeast", "Southeast", "South-southeast", "South",
  "South-southwest", "Southwest", "West-southwest", "West", "West-northwest", "Northwest", "North-northwest", "North"
};

//Seriale Stazione Meteo
AltSoftSerial MeteoSerial;

//---------------------------------------------------------------------------
/////////////////////////////////// SETUP ///////////////////////////////////
//---------------------------------------------------------------------------

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  //Led setup
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  
  /**************************** Setup ANEMOMEMTRO  ******************************/ 

  digitalWrite(12, HIGH);
  while ( !( windDirection.begin() ) ) {
    Serial.println("Communication with device failed, please check connection");
    delay(3000);
  }
  Serial.println("Begin ok!");
  delay(500);
  
  /**************************** Setup TERMOMETRO  ******************************/
  
  digitalWrite(11, HIGH);
  while (sht3x.begin() != 0) {
    Serial.println("Failed to Initialize the chip, please confirm the wire connection");
  }
  Serial.print("Chip serial number");
  Serial.println(sht3x.readSerialNumber());
  if(!sht3x.softReset()){
    Serial.println("Failed to Initialize the chip....");
  }
  delay(500);

  /**************************** Setup PLUVIOMETRO ******************************/

  pinMode(4, INPUT);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  
  /**************************** Setup MeteoSerial ******************************/

  digitalWrite(10, HIGH);
  delay(500);
  MeteoSerial.begin(9600);
 
  /**************************** Setup COMPLETO ******************************/

  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  Serial.println("setup completato");
  
}

//---------------------------------------------------------------------------
/////////////////////////////////// LOOP ////////////////////////////////////
//---------------------------------------------------------------------------

void loop(){

//---------------------------------------------------------------------------
////////////////////////////////// RESET VARIABILI //////////////////////////
//---------------------------------------------------------------------------

  Pioggia = 0;
  T       = 0;
  U       = 0;
  I       = 0;
  media   = 0;
  conteggio        = false;
  copie            = false;
  ConteggioSensori = false;
  //Timer While
  t1 = millis();
  dt = millis()-t1;
  //Timer lettura sensori
  ts  = millis();
  dts = millis()-ts;

  //---------------------------------------------------------------------------
  /////////////////////////////////// PLUVIOMETRO /////////////////////////////
  //---------------------------------------------------------------------------

  while(dt < 299000 ){
    //Serial.println("osservo il pluviometro");
    sensorValue = digitalRead(4);
    //Serial.println(sensorValue);
    if(sensorValue == 0 && conteggio == false){ //il pluviometro scatta
      tp = millis();
      conteggio = true;
    } 
    
    if(conteggio == true){
      dtp = millis() - tp;
      if(dtp < 450 && copie == false){
        goccia = 1;
        Pioggia = Pioggia + goccia;
        copie = true;
        //Serial.println("scatto contato");
      }
      if(dtp >= 450){
        tp = 0;
        conteggio = false;
        copie = false;
        //Serial.println("Reset e somma goccie");
      } 
    }

    dts = millis() - ts;
    if(dts < 59700 && ConteggioSensori == false){
      
      //--------------------------------------------------------------------------
      /////////////////////////////////// TERMOMETRO /////////////////////////////
      //--------------------------------------------------------------------------
      //Serial.println("leggo i sensori");
      
      Tvalue = sht3x.getTemperatureC();
      T = T + Tvalue;
      delay(10);
    
      Uvalue = sht3x.getHumidityRH(); 
      U = U + Uvalue;
      delay(10);
    
      //------------------------------------------------------------------------
      ///////////////////////////////// ANEMOMETRO /////////////////////////////
      //------------------------------------------------------------------------
    
      //intensità vento m/s
      sensor2Value = analogRead(A0);
      outvoltage = sensor2Value * (5.0 / 1023.0);
      Level = 6 * outvoltage;
      I = I + Level;
      delay(10);

      ConteggioSensori = true;
      media++;
    }
    
    if (dts >= 59700){
      ConteggioSensori = false;
      ts = millis();
      //Serial.println("pronto a rileggere i sensori");
    }
    
    //Serial.println(Pioggia);
    
    //Serial.println(Tvalue);
    
    //Serial.println(Uvalue);
    
    //Serial.println(media);
    dt  = millis() - t1;
  }

  //---------------------------------------------------------------------------
  ////////////////////////////////// MEDIE ////////////////////////////////////
  //---------------------------------------------------------------------------

  Temperatura = T/media;
  Umidita     = U/media;
  Inensita    = 3.6 * I/media;
  Direction   = windDirection.GetWindDirection(/*modbus slave address*/Address);
  delay(30);
  
  //---------------------------------------------------------------------------
  ////////////////////////////////// Seriale METEO ////////////////////////////
  //---------------------------------------------------------------------------
  
  MeteoSerial.print(Temperatura);
  MeteoSerial.print(",");
  MeteoSerial.print(Umidita);
  MeteoSerial.print(",");
  MeteoSerial.print(Orientation[Direction]);
  MeteoSerial.print(",");
  MeteoSerial.print(Inensita);
  MeteoSerial.print(",");
  MeteoSerial.println(Pioggia);
  delay(100); 
}
