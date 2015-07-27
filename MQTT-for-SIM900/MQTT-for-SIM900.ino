/*
 Very, very primitive. MQTT Test program to send data to a MQTT Server

 V00 / 09th March 2013, omer sever, omers@tr.ibm.com
 initial version w/o valid IOC parameters

 V01 / 10th March 2013, ates yurdakul, atesh@puaga.com
 modified for IOC parameters
 disabled GPS for simplicity
 sending "static" information for simplicity
*/


#include <mqtt.h>
#include <Math.h>
//#include <SoftwareSerial.h>
#include <String.h>
#include "DHT.h"

/* #include <SoftwareSerial.h>
#include "TinyGPS.h"
TinyGPS gps;
*/
// initialize the library with the numbers of the interface pins
//variable for Water temperature
// which analog pin to connect
#define THERMISTORPIN A2         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000

int samples[NUMSAMPLES];


#define DHTPIN A0    //Analog Pin 0 for AM2301
#define DHTTYPE DHT21// AM2301 

DHT dht(DHTPIN, DHTTYPE);
float Air_Temp = 0.0; //AM2301 temparature in celcus
float Air_Humidity = 0.0; // AM2301 Humidity 
int ldr = 1;             //analog pin to which LDR is connected
int ldr_value = 0;        //variable to store LDR values

// GPS parser for 406a
#define BUFFSIZ 90 // plenty big
char buffer[BUFFSIZ];
char buffidx;
char *parseptr, *initptr;
uint8_t hour, minute, second, year, month, date;
uint32_t latitude, longitude;
uint8_t groundspeed, trackangle;
char latdir, longdir;
char status;
int i = 1;
char atCommand[50];
byte mqttMessage[127];
int mqttMessageLength = 0;
String gsmStr = "";
String gprsStr = "";
char mqttSubject[50];
int index = 0;
byte data1;
boolean smsReady = false;
boolean smsSent = false;
boolean gprsReady = false;
boolean mqttSent = false;
int sentCount = 0;
// SoftwareSerial gpss(1, 3);



void setup() {
 pinMode(13, OUTPUT);
 pinMode(14, OUTPUT); // GSM
 digitalWrite(0, HIGH);


 Serial.begin(9600); //debug
 Serial1.begin(19200); // GPRS module
// gpss.begin(9600);
 dht.begin();

}
void loop(){
 delay(20000);// wait for GPRS getting stable
 get_AM2301_Data();
 
 Serial.println("Checking if GPRS is ready");
 gprsReady = isGPRSReady();
 
 if (gprsReady == true){
 Serial.println("GPRS Ready");
 String json = buildJson();
 char jsonStr[200];
 json.toCharArray(jsonStr,200);
    
 // Change the IP and Topic.
 /* The arguments here are:
 clientID, IP, Port, Topic, Message
 */
sendMQTTMessage("agrinode", "iot.eclipse.org", "1883", "agrinode1",jsonStr);
 }

 delay(10000);

}


uint32_t parsedecimal(char *str) {
 uint32_t d = 0;
 while (str[0] != 0) {
 if ((str[0] > '9') || (str[0] < '0'))
 return d;
 d *= 10;
 d += str[0] - '0';
 str++;
 }
 return d;
}

void readline() {
 /*char c;
 buffidx = 0; // start at begninning
 while (1) {
 c = gpss.read();
 if (c == -1)
 continue;
 Serial.print(c);
 if (c == '\n')
 continue;
 if ((buffidx == BUFFSIZ-1) || (c == '\r')) {
 buffer[buffidx] = 0;
 return;
 }
 buffer[buffidx++]= c;
 }*/
}
boolean isGPRSReady(){
 Serial1.println("AT+CGATT?");
 index = 0;
 while (Serial1.available()){
 data1 = (char)Serial1.read();
 Serial.write(data1);
 gprsStr[index++] = data1;
 }
 Serial.println("Check OK");
 Serial.print("gprs str = ");
 Serial.println(data1);
 if (data1 > -1){
 Serial.println("GPRS OK");
 return true;
 }
 else {
 Serial.println("GPRS NOT OK");
 return false;
 }
}

void sendMQTTMessage(char* clientId, char* brokerUrl, char* brokerPort, char* topic, char* message){
 Serial1.println("AT"); // Sends AT command to wake up cell phone
 Serial.println("send AT to wake up GPRS");
 delay(1000); // Wait a second
 digitalWrite(13, HIGH);
 Serial1.println("AT+CSTT=\"m-wap\",\"mms\",\"mms\""); // Puts phone into GPRS mode
 Serial.println("AT+CSTT=\"m-wap\",\"mms\",\"mms\"");
 delay(2000); // Wait a second
 Serial1.println("AT+CIICR");
 Serial.println("AT+CIICR");
 delay(2000);
 Serial1.println("AT+CIFSR");
 Serial.println("AT+CIFSR");
 delay(2000);
 strcpy(atCommand, "AT+CIPSTART=\"TCP\",\"");
 strcat(atCommand, brokerUrl);
 strcat(atCommand, "\",\"");
 strcat(atCommand, brokerPort);
 strcat(atCommand, "\"");
 Serial1.println(atCommand);
 Serial.println(atCommand);
 // Serial.println("AT+CIPSTART=\"TCP\",\"mqttdashboard.com\",\"1883\"");
 delay(2000);
 Serial1.println("AT+CIPSEND");
 Serial.println("AT+CIPSEND");
 delay(2000);
 mqttMessageLength = 16 + strlen(clientId);
 Serial.println(mqttMessageLength);
 mqtt_connect_message(mqttMessage, clientId);
 for (int j = 0; j < mqttMessageLength; j++) {
 Serial1.write(mqttMessage[j]); // Message contents
 Serial.write(mqttMessage[j]); // Message contents
 Serial.println("");
 }
 Serial1.write(byte(26)); // (signals end of message)
 Serial.println("Sent");
 delay(10000);
 Serial1.println("AT+CIPSEND");
 Serial.println("AT+CIPSEND");
 delay(2000);
 mqttMessageLength = 4 + strlen(topic) + strlen(message);
 Serial.println(mqttMessageLength);
 mqtt_publish_message(mqttMessage, topic, message);
 for (int k = 0; k < mqttMessageLength; k++) {
 Serial1.write(mqttMessage[k]);
 Serial.write((byte)mqttMessage[k]);
 }
 Serial1.write(byte(26)); // (signals end of message)
 Serial.println("-------------Sent-------------"); // Message contents
 delay(5000);
 Serial1.println("AT+CIPCLOSE");
 Serial.println("AT+CIPCLOSE");
 delay(2000);
}

//get AM2301 data function
void get_AM2301_Data() { 
    Serial.println("Read data"); 
   Air_Humidity = dht.readHumidity();
   Air_Temp = dht.readTemperature();
   
   if (isnan(Air_Humidity) || isnan(Air_Temp)) {
    Serial.println("Failed to read from DHT sensor");
    Air_Humidity=0;
    Air_Temp=0;
    return;
  }
}
  
String buildJson() {
  String data = "{";
  data+="\n";
  data+= "\"d\": {";
  data+="\n";
  data+="\"myName\": \"Arduino DHT11\",";
  data+="\n";
  data+="\"Air Temperature\": ";
  data+=(int)Air_Temp;
  data+= ",";
  data+="\n";
  data+="\"Air Humidity\": ";
  data+=(int)Air_Humidity;
  data+="\n";
  data+="}";
  data+="\n";
  data+="}";
  return data;
}
