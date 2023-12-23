#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "time.h"
#include <TinyGsmClient.h>
#include "utilities.h"
#include "arduino_secrets.h"


// WiFi config
const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASSWORD;

// HTTPS connection config
const char* host = SECRET_TRACCAR_HOST;
const int httpsPort = 443;
const char* resource = "/";
// Traccar ID
String id = SECRET_TRACCAR_ID;

// Accuracy seettings
float degreemargin = 0.10;
float accuracymargin = 4.00;
// should be kept at the initial value of 0.00
float previouslat = 0.00;
float previouslon = 0.00;

// NTP server to request epoch time
const char* ntpServer = "be.pool.ntp.org";

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

TinyGsm modem(SerialAT);

// Function to validate coordinates and accuracy
void verify_data(float lat, float lon, float speed, float alt, float accuracy) {
  int sendit = 0;
  Serial.print("Sendit value is set to: ");
  Serial.println(sendit);
  // check if we are still using the initial lat/lon previous values
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
      Serial.println("Accuracy Not OK");
    }
  } else {
    Serial.println("Not using initial GPS coordinates");
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
        Serial.println("Coordinates between margins");
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

  // Use WiFiClientSecure class to create a TLS connection
  WiFiClientSecure client;
  client.setInsecure();  // Set to use an insecure connection

  Serial.print("Connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    delay(10000); // Wait for 10 seconds before retrying
    return;
  }

  // Make a HTTP POST request
  epochTime = getTime();
  String postData = "id=" + id + "&lat=" + String(lat,6) + "&lon=" + String(lon,6) + "&timestamp=" + String(epochTime) + "&altitude=" + String(alt,6) + "&speed=" + String(speed,6); // Adjust the data to be sent
  String postRequest =
      "POST " + String(resource) + " HTTP/1.1\r\n" +
      "Host: " + String(host) + "\r\n" +
      "Connection: close\r\n" +
      "Content-Type: application/x-www-form-urlencoded\r\n" +
      "Content-Length: " + String(postData.length()) + "\r\n" +
      "\r\n" +
      postData;

  Serial.println("Sending POST request...");

  client.print(postRequest);

  // Wait for the response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers received");
      break;
    }
  }

  // Print the response
  while (client.available()) {
    Serial.println("Response:");
    Serial.write(client.read());
    Serial.print("\n");
  }

  Serial.println("Closing connection");
  delay(1000); // Wait for 1 seconds before making the next request
}

// Function to set up WiFi
void setup_wifi() {
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to set up time
void setup_time() {
  // Configuring time
  configTime(0, 0, ntpServer);
}

// Function to set up the GPS module
void setup_gps() {
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

    Serial.println("Start modem...");


    for (int i = 0; i < 3; ++i) {
        while (!modem.testAT(5000)) {
            Serial.println("Try to start modem...");
            pinMode(MODEM_PWRKEY, OUTPUT);
            digitalWrite(MODEM_PWRKEY, HIGH);
            delay(300); //Need delay
            digitalWrite(MODEM_PWRKEY, LOW);
        }
    }

    Serial.println("Modem Response Started.");

    // Stop GPS Server
    modem.sendAT("+CGPS=0");
    modem.waitResponse(30000);

    // Configure GNSS support mode
    modem.sendAT("+CGNSSMODE=15,1");
    modem.waitResponse(30000);

    // Configure NMEA sentence type
    modem.sendAT("+CGPSNMEA=200191");
    modem.waitResponse(30000);

    // Set NMEA output rate to 1HZ
    modem.sendAT("+CGPSNMEARATE=1");
    modem.waitResponse(30000);

    // Enable GPS
    modem.sendAT("+CGPS=1");
    modem.waitResponse(30000);
}

// Setup function
void setup() {
  setup_wifi();
  setup_gps();
  setup_time();
}

// Main loop
void loop() {
  float lat, lon, speed, alt, accuracy;
  int vsat, usat;
  while (1) {
    if (modem.getGPS(&lat, &lon, &speed, &alt, &vsat, &usat, &accuracy)) {
      verify_data(lat, lon, speed, alt, accuracy);
      // send_data(lat, lon, speed, alt, accuracy);
    }
    delay(5);
  }
}
