#include "DHT.h"
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "time.h"
#include "math.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1800        /* Time ESP32 will go to sleep (in seconds) */


RTC_DATA_ATTR int failedCount = 0;
RTC_DATA_ATTR int failedData[2][48];
RTC_DATA_ATTR int prevTime[] = {2020, 12, 31, 22, 10};

RTC_DATA_ATTR int monthEnds[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define FIREBASE_HOST "hydro-3a118.firebaseio.com"
#define FIREBASE_AUTH "CdQchiXp42XPzShG42ZjRmDe5b8pPcgDyLBFDvMP"
#define WIFI_SSID "SLT-ADSL-17022"
#define WIFI_PASSWORD "ran2280252"

#define DHTPIN 4     // Digital pin connected to the DHT sensor

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);


//Define FirebaseESP32 data object
FirebaseData firebaseData;
FirebaseJson json;
String dbPath = "";

int temperature = 0;
int  humidity = 0;

unsigned long mainCounter = 0;
unsigned long activeTime = 20000;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 3600;

int Krish_day;
int Krish_month;
int  Krish_year;

int Krish_hour;
int Krish_min;
int  Krish_sec;

struct tm timeinfo;

void setup() {
//  Serial.begin(115200);




// testFailCount();

  /*
    First we configure the wake up source
    We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");
  dht.begin();

  initFirebase();



  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  updateTime();

  float temp = readTemp();
  float humid = readHumid();
  saveToFirebase(Krish_year , Krish_month , Krish_day , Krish_hour , Krish_min, temp , humid);



  reSaveFailed();
  resetFailure();
  esp_deep_sleep_start();
}

void loop() {
  //This is not going to be called




}

void sleep() {

}

void initFirebase() {

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");


  while (WiFi.status() != WL_CONNECTED)
  {
    //    Serial.print(".");
    delay(300);
    checkSleepOverflow();
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(false);



  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");


}

void saveToFirebase(int year , int month , int day , int hour , int mint, float temp , float humid) {



  json.set("/temp", String(temp, 2));
  json.set("/humid", String(humid, 2));
  dbPath = "";
  dbPath += "/Sensor/data/";
  dbPath += String(year);
  dbPath += "-";
  dbPath += String(month);
  dbPath += "-";
  dbPath += String(day);
  dbPath += "/";
  dbPath += String(hour);
  dbPath += ":";
  dbPath += String(mint);
  dbPath += "/";

  Serial.println(dbPath);
  Firebase.updateNode(firebaseData, dbPath, json);

}

void checkSleepOverflow() {
  Serial.println(mainCounter);
  Serial.println(millis());
  mainCounter = millis();

  if ( mainCounter > activeTime) {
    Serial.println("Going to sleep now ");
    Serial.flush();
    logFailure();
    esp_deep_sleep_start();
  }

}




float readTemp() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();


  // Check if any reads failed and exit early (to try again).
  while ( isnan(t) ) {

    Serial.println(F("Failed to read from DHT sensor!"));
    checkSleepOverflow();
    delay(1000);
    t = dht.readTemperature();
  }
  Serial.print("temp:"); Serial.println(t);

  return t;
}
float readHumid() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();


  // Check if any reads failed and exit early
  while ( isnan(h) ) {

    Serial.println(F("Failed to read from DHT sensor!"));
    checkSleepOverflow();
    delay(1000);
    h = dht.readHumidity();
  }
  Serial.print("Humid:"); Serial.println(h);

  return h;
}




void  resetFailure() {

  prevTime[0] = Krish_year ;
  prevTime[1] = Krish_month ;
  prevTime[2] = Krish_day ;
  prevTime[3] = Krish_hour ;
  prevTime[4] = Krish_min ;
  failedCount = 0 ;
}

void  logFailure() {
  failedData[0][failedCount] = (int)(readTemp()*100);
  failedData[1][failedCount] = (int)(readHumid()*100);



  failedCount++;

  if (failedCount >= 23) {
    failedCount = 23;
  }
  Serial.println("logged failures");

}
//RTC_DATA_ATTR int failedCount = 0;
//RTC_DATA_ATTR byte failedData[2][24];
//RTC_DATA_ATTR int prevTime[] = {2020,8,11,7};
void reSaveFailed() {

  for (int i = 0 ; i < failedCount ; i++) {
    int mint = prevTime[4];
    int hour = prevTime[3];
    int day = prevTime[2];
    int month = prevTime[1];
    int year = prevTime[0];

    mint += (TIME_TO_SLEEP * (i + 1)) / 60 ;

    if (mint >= 60) {
      hour += mint / 60;

      if (hour >= 24) {

        day += hour / 24 ;


        if ( day > monthEnds[month - 1]) {
          
          day -= monthEnds[month - 1];
          
          month += 1 ;

          if (month > 12) {

            year += 1 ;
            month = 1 ;
          }
 


        }
        hour = hour % 24;
      }
      mint = mint % 60;
    }


    Serial.print(mint); Serial.print(":"); Serial.print(hour); Serial.print(":"); Serial.print(day); Serial.print(":"); Serial.print(month); Serial.print(":"); Serial.print(year); Serial.print(":"); Serial.println();


    saveToFirebase(year, month, day, hour, mint , (float)(failedData[0][i])/100 , (float)(failedData[1][i])/100);


  }

}
void updateTime() {

  while (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    checkSleepOverflow();
    delay(300);
  }
  Krish_hour = timeinfo.tm_hour;
  Krish_min  = timeinfo.tm_min;
  Krish_sec  = timeinfo.tm_sec;

  Krish_day = timeinfo.tm_mday;
  Krish_month = timeinfo.tm_mon + 1;
  Krish_year = timeinfo.tm_year + 1900;


  Serial.print(Krish_hour); Serial.print(":"); Serial.print(Krish_min); Serial.print(":"); Serial.print(Krish_sec); Serial.print(":"); Serial.println();
  Serial.print(Krish_day); Serial.print(":"); Serial.print(Krish_month); Serial.print(":"); Serial.print(Krish_year); Serial.print(":"); Serial.println();
}

double round_to_dp( float in_value, int decimal_place )
{
  int multiplier =  100;
  Serial.println(multiplier, 9);
  Serial.println(roundf( in_value * multiplier ), 9);
  in_value = roundf( in_value * multiplier ) / multiplier;
  Serial.println(in_value, 9);
  return (double)in_value;
}

void testFailCount(){
  
    failedCount = 4 ;

  failedData[0][0] = 2030;
  failedData[0][1] = 2132;
  failedData[0][2] = 2200;
  failedData[0][3] = 2332;

  failedData[1][0] = 4043;
  failedData[1][1] = 4122;
  failedData[1][2] = 4200;
  failedData[1][3] = 4311;



  
  
  }

