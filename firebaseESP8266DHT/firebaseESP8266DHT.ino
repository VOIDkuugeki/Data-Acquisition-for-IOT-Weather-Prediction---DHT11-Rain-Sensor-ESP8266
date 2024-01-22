#include <TimeLib.h> 
#include <NTPClient.h>
#include <WiFiUdp.h>

const char *ntpServer = "0.asia.pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; // Vietnam GMT+7
const int daylightOffset_sec = 0; // Daylight offset in seconds (if applicable)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);


#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
#include "DHT.h"

//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#define DHTPIN D2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

#define RSPIN A0      // Digital pin connected to the Rain sensor

/* Define the WiFi credentials */
#define WIFI_SSID "name_of_wifi"
#define WIFI_PASSWORD "password"

/* Define the API Key */
#define API_KEY "AIzaSyBiKubcoCrc9eHzAc4HRJPMTqOhtZZHZtA"

/* Define the RTDB URL */
#define DATABASE_URL "rtdb_url" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* Define the user Email and password that already registered or added in your project */
#define USER_EMAIL "email@mail"
#define USER_PASSWORD "password"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  dht.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);

  Serial.println(Firebase.ready() ? "Firebase connected" : "Firebase not connected");

  Firebase.setDoubleDigits(5);

  // Synchronize time
  Serial.print("Synchronizing time");
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Time synchronized: ");
  Serial.println(timeClient.getFormattedTime());
}

void loop()
{
  // Get the current time in seconds after synchronization  
  time_t currentTime = timeClient.getEpochTime();

  double r = analogRead(RSPIN);
  r = map(r, 0, 1024, 0, 100);

  float h = dht.readHumidity();
  float t = dht.readTemperature();



  if (isnan(h) || isnan(t))                                     // Checking sensor working
  {                                   
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    // Convert the current time to a formatted string (YYYY-MM-DD HH:MM:SS)
    char dateTime[20];
    sprintf(dateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            year(currentTime), month(currentTime), day(currentTime),
            hour(currentTime), minute(currentTime), second(currentTime));

    // Create a data path with the formatted date and time as the counter
    String dataPath = "/sensor/data/" + String(dateTime);

    // Set Temperature
    Serial.printf("Set Temperature... %s\n", Firebase.setFloat(fbdo, dataPath + "/temperature", t) ? "ok" : fbdo.errorReason().c_str());

    // Get Temperature
    Serial.printf("Get Temperature... %s\n", Firebase.getFloat(fbdo, dataPath + "/temperature") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());

    // Set Humidity
    Serial.printf("Set Humidity... %s\n", Firebase.setDouble(fbdo, dataPath + "/humidity", h) ? "ok" : fbdo.errorReason().c_str());

    // Get Humidity
    Serial.printf("Get Humidity... %s\n", Firebase.getDouble(fbdo, dataPath + "/humidity") ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());

    // Set Rain
    Serial.printf("Set Rain... %s\n", Firebase.setDouble(fbdo, dataPath + "/rain", r) ? "ok" : fbdo.errorReason().c_str());

    // Get Rain
    Serial.printf("Get Rain... %s\n", Firebase.getDouble(fbdo, dataPath + "/rain") ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());

    Serial.println();
    
  }

}
