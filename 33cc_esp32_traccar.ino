#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

#define USE_WIFI false
#define USE_GPRS true

#if USE_WIFI
#include <WiFi.h>
#endif

#include "Time.h"
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include "utilities.h"
#include "arduino_secrets.h"
#include <ESP32Time.h>

// Date/Time Config
ESP32Time rtc(3600);

TinyGsm modem(SerialAT);
#if USE_GPRS
// Mobile config
const char gprsApn[]  = SECRET_GPRS_APN;
const char gprsUser[] = SECRET_GPRS_USER;
const char gprsPass[] = SECRET_GPRS_PASS;

// Initialize TinyGSM
TinyGsmClient gprs(modem);
#endif

#if USE_WIFI
// WiFi config
WiFiClient wifi;
const char* wifiSSID = SECRET_WIFI_SSID;
const char* wifiPass = SECRET_WIFI_PASS;
#endif

#if USE_WIFI
HttpClient client = HttpClient(wifi, SECRET_TRACCAR_HOST, SECRET_TRACCAR_PORT);
#endif

#if USE_GPRS
HttpClient client = HttpClient(gprs, SECRET_TRACCAR_HOST, SECRET_TRACCAR_PORT);
#endif

// Traccar ID
String id = SECRET_TRACCAR_ID;

// Accuracy settings for GPS reporting
float degreemargin = 0.10;
float accuracymargin = 4.00;
// should be kept at the initial value of 0.00
float previouslat = 0.00;
float previouslon = 0.00;

// Variable to save current epoch time
unsigned long epochTime;

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

// Function to validate coordinates and accuracy
void verify_data(float lat, float lon, float speed, float alt, float accuracy) {
  int sendit = 0;
  Serial.print("Sendit value is set to: ");
  Serial.println(sendit);
  // check if we are still using the initial lat/lon values
  if (( previouslat == 0.00f ) or ( previouslon == 0.00f )) {
    Serial.println("Using initial GPS coordinates");
    Serial.println("Checking Accuracy..");
    Serial.print("Accuracy is: ");
    Serial.println(accuracy);
    if (accuracy <= accuracymargin) {
      Serial.print("lat is: ");
      Serial.println(lat,6);
      Serial.print("lon is: ");
      Serial.println(lon,6);
      sendit = 1;
      previouslat = lat;
      previouslon = lon;
      Serial.println("Accuracy OK");
    } else {
      Serial.println("Accuracy is not OK");
    }
  // We are not using initial lat/lon values
  } else {
    Serial.println("Checking Accuracy");
    Serial.print("Accuracy is: ");
    Serial.println(accuracy);
    if (accuracy <= accuracymargin) {
      float latmarginh = (previouslat + degreemargin);
      float latmarginl = (previouslat - degreemargin);
      float lonmarginh = (previouslon + degreemargin);
      float lonmarginl = (previouslon - degreemargin);
      Serial.print("lat is: ");
      Serial.println(lat,6);
      Serial.print("lat high margin is: ");
      Serial.println(latmarginh,6);
      Serial.print("lat low margin is: ");
      Serial.println(latmarginl,6);
      Serial.print("lon is: ");
      Serial.println(lon,6);
      Serial.print("lon high margin is: ");
      Serial.println(lonmarginh,6);
      Serial.print("lon low margin is: ");
      Serial.println(lonmarginl,6);
      if ((lat > latmarginl) && (lat < latmarginh) && (lon > lonmarginl) && (lon < lonmarginh)) {
        Serial.println("Coordinates are between margins");
        sendit = 1;
        previouslat = lat;
        previouslon = lon;
      }
    }
  }
  if (sendit == 1) {
    Serial.println("Sending data");
    send_data(lat, lon, speed, alt, accuracy);
  }
}

// Function to send GPS data to the server
void send_data(float lat, float lon, float speed, float alt, float accuracy) {

  epochTime = getTime();

  Serial.println("making POST request");
  String postData = "id=" + id + "&lat=" + String(lat,6) + "&lon=" + String(lon,6) + "&timestamp=" + String(epochTime) + "&altitude=" + String(alt,6) + "&speed=" + String(speed,6);

  client.beginRequest();
  client.post("/");
  client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
  client.sendHeader("Content-Length", postData.length());
  client.beginBody();
  client.print(postData);
  client.endRequest();

  // read the status code
  int statusCode = client.responseStatusCode();

  Serial.print("Status code: ");
  Serial.println(statusCode);
}

#if USE_WIFI
// Function to set up WiFi
void setup_wifi() {
  // Connect to Wi-Fi
  WiFi.begin(wifiSSID, wifiPass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}
#endif

#if USE_GPRS
// Function to set up GPRS
void setup_gprs() {
  // Unlock your SIM card with a PIN if needed
  if (SECRET_SIM_PIN && modem.getSimStatus() != 3) {
    Serial.println("Entering SIM PIN");
    modem.simUnlock(SECRET_SIM_PIN);
  }

  // GPRS connection parameters are usually set after network registration
  Serial.print(F("Connecting to "));
  Serial.print(gprsApn);
  while (!modem.gprsConnect(gprsApn, gprsUser, gprsPass)) {
    Serial.print(".");
    delay(30000);
  }
  Serial.println(" OK");
}
#endif

// Variables for GPS
float lat, lon, speed, alt, accuracy;
int vsat, usat;
int year, month, day, hour, minute, second;

// Function to set up time
void setup_time() {
  Serial.print(F("Waiting for GPS Data for time config"));
  while (!modem.getGPS(&lat, &lon, &speed, &alt, &vsat, &usat, &accuracy, &year, &month, &day, &hour, &minute, &second)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" OK");
  rtc.setTime(second, minute, hour, day, month, year);
}

// Function to set up the Modem
void setup_modem() {
    Serial.begin(115200); // Set console baud rate
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

    /*
    The indicator light of the board can be controlled
    */
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    /*
    MODEM_PWRKEY IO:4 The power-on signal of the modulator must be given to it,
    otherwise the modulator will not reply when the command is sent
    */
    pinMode(MODEM_PWRKEY, OUTPUT);
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(300); //Need delay
    digitalWrite(MODEM_PWRKEY, LOW);

    /*
    MODEM_FLIGHT IO:25 Modulator flight mode control,
    need to enable modulator, this pin must be set to high
    */
    pinMode(MODEM_FLIGHT, OUTPUT);
    digitalWrite(MODEM_FLIGHT, HIGH);

    Serial.print("Starting modem");


    for (int i = 0; i < 3; ++i) {
        while (!modem.testAT(5000)) {
            Serial.print(".");
            pinMode(MODEM_PWRKEY, OUTPUT);
            digitalWrite(MODEM_PWRKEY, HIGH);
            delay(300); //Need delay
            digitalWrite(MODEM_PWRKEY, LOW);
        }
    }
    Serial.println(" OK");
}

// Function to set up the GPS module
void setup_gps() {

    Serial.print("Starting GPS module");
    // Stop GPS Server
    modem.sendAT("+CGPS=0");
    modem.waitResponse(30000);

    Serial.print(".");
    // Configure GNSS support mode
    modem.sendAT("+CGNSSMODE=15,1");
    modem.waitResponse(30000);

    Serial.print(".");
    // Configure NMEA sentence type
    modem.sendAT("+CGPSNMEA=200191");
    modem.waitResponse(30000);

    Serial.print(".");
    // Set NMEA output rate to 1HZ
    modem.sendAT("+CGPSNMEARATE=1");
    modem.waitResponse(30000);

    Serial.print(".");
    // Enable GPS
    modem.sendAT("+CGPS=1");
    modem.waitResponse(30000);

    Serial.println(" OK");
}

// Setup function
void setup() {
  //delay a little so we can follow
  delay(3000);
  setup_modem();
  setup_gps();
  setup_time();

#if USE_WIFI
  setup_wifi();
#endif

#if USE_GPRS
  setup_gprs();
#endif

}

// Main loop
void loop() {
  if (modem.getGPS(&lat, &lon, &speed, &alt, &vsat, &usat, &accuracy)) {
    verify_data(lat, lon, speed, alt, accuracy);
  }
  delay(5000);
}
