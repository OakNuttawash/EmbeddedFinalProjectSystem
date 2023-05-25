#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <WiFiManager.h>
#include <SoftwareSerial.h>
#include <NTPClient.h>


//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

WiFiManager wifiManager;

// Insert your network credentials
#define WIFI_SSID "p"
#define WIFI_PASSWORD "Peampeam"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAafMMDt569mI3kZvtHm4dJFNHeRprgAZs"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://embeded-final-project-default-rtdb.asia-southeast1.firebasedatabase.app/"



//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int intValue;
float floatValue;
int count = 0;
bool signupOK = false;

const char* ssid = WIFI_SSID;
const char* password = WIFI_SSID;

// Define the UART pins
#define NODEMCU_TX_PIN D1
#define NODEMCU_RX_PIN D2

// Create a SoftwareSerial object
SoftwareSerial NodeMCUSerial(NODEMCU_TX_PIN, NODEMCU_RX_PIN);

// Define NTP Client to get time
const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);


void setup() {
  NodeMCUSerial.begin(115200);
  Serial.begin(115200); 
  timeClient.begin();
  Serial.println("waiting for connecting wifi...");
  wifiManager.autoConnect();
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  Serial.println("connecting to firebase...");
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("start...");
  delay(2000);

}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void loop() {
  timeClient.update();
  if (Firebase.ready() && signupOK  && NodeMCUSerial.available()) {
    String receivedData = NodeMCUSerial.readString();
    receivedData.trim();
    String value =  receivedData + ",S";
    Serial.println(value);
    String data[5] ;
    for ( int i = 0 ; i < 4 ;i++){
      data[i] = getValue(value, ',', i);
    }
    String CO = data[0];
    String Temp = data[1];
    String Humid = data[2];
    String PM = data[3];
    //Serial.println(data[2]);

    //Hour:minute:second//
    String time;
    if (timeClient.getMinutes() < 10 && timeClient.getSeconds() < 10) time = String(timeClient.getHours()) + ':' + '0' + String(timeClient.getMinutes()) + ':' + '0' + String(timeClient.getSeconds()) ;
    else if ( timeClient.getMinutes() < 10 ) time = String(timeClient.getHours()) + ':' + '0' + String(timeClient.getMinutes()) + ':' +  String(timeClient.getSeconds()) ;
    else if ( timeClient.getSeconds() < 10 ) time = String(timeClient.getHours()) + ':' + String(timeClient.getMinutes()) + ':' + '0' + String(timeClient.getSeconds()) ;
    else time = String(timeClient.getHours()) + ':' + String(timeClient.getMinutes()) + ':' + String(timeClient.getSeconds()) ;
    int time2 = timeClient.getHours()*10000 + timeClient.getMinutes()*100 + timeClient.getSeconds();
    
    //date-month-year//
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime); 
    int monthDay = ptm->tm_mday ;
    int currentMonth = ptm->tm_mon+1;
    int currentYear = ptm->tm_year+1900;
    String currentDate;
    if ( monthDay < 10 && currentMonth < 10) currentDate =  String(currentYear) + '-' + '0' + String(currentMonth) + '-' + '0' +String(monthDay);
    else if ( currentMonth < 10) currentDate =  String(currentYear) + '-' + '0' + String(currentMonth) + '-' + String(monthDay);
    else if ( monthDay < 10) currentDate =  String(currentYear) + '-' +  String(currentMonth) + '-' + '0' + String(monthDay);
    else currentDate =  String(currentYear) + '-' +  String(currentMonth) + '-' +  String(monthDay);

    // Write an data on the database
    Firebase.RTDB.setInt(&fbdo, "data/"+currentDate+'/'+time+"/Time", time2);
    Firebase.RTDB.setString(&fbdo, "data/"+currentDate+'/'+time+"/CO", CO);
    Firebase.RTDB.setString(&fbdo, "data/"+currentDate+'/'+time+"/Humid", Humid);
    Firebase.RTDB.setString(&fbdo, "data/"+currentDate+'/'+time+"/temp", Temp);

  }
  
  
}
