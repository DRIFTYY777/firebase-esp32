#include <Arduino.h>

#include <SPIFFS.h>
#include <WiFiSettings.h>
#include <ArduinoOTA.h>

#include "addons/TokenHelper.h"
#include <Firebase_ESP_Client.h>

#include "DHT.h"

#define API_KEY "AIzaSyBfppxm4qqUo60xeV2k7Ztr9z2qHc0lT6I"
#define FIREBASE_PROJECT_ID "that-esp"
#define USER_EMAIL "gbjhasd344@gmail.com"
#define USER_PASSWORD "mmmmmmmmmm777"
#define DOOR_SENSOR_PIN 19 // ESP32 pin GPIO19 connected to door sensor's pin

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long dataMillis = 0;
int count = 0;
int doorState;

DHT dht;

void fcsUploadCallback(CFS_UploadStatusInfo info)
{
  if (info.status == fb_esp_cfs_upload_status_init)
  {
    Serial.printf("\nUploading data (%d)...\n", info.size);
  }
  else if (info.status == fb_esp_cfs_upload_status_upload)
  {
    Serial.printf("Uploaded %d%s\n", (int)info.progress, "%");
  }
  else if (info.status == fb_esp_cfs_upload_status_complete)
  {
    Serial.println("Upload completed ");
  }
  else if (info.status == fb_esp_cfs_upload_status_process_response)
  {
    Serial.print("Processing the response... ");
  }
  else if (info.status == fb_esp_cfs_upload_status_error)
  {
    Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
  }
}

void firebase_init()
{
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void write_data(double temp, double humi)
{
  if (Firebase.ready() && (millis() - dataMillis > 60000 || dataMillis == 0))
  {
    dataMillis = millis();
    FirebaseJson content;
    String documentPath = "kharwan/Roof_1";

    content.set("fields/temperature/doubleValue", String(temp).c_str());
    content.set("fields/humidity/doubleValue", String(humi).c_str());

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "temperature,humidity"))
    {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      return;
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw()))
    {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      return;
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
}

void setup_ota()
{
  ArduinoOTA.setHostname(WiFiSettings.hostname.c_str());
  ArduinoOTA.setPassword(WiFiSettings.password.c_str());
  ArduinoOTA.begin();
}

void redswitch()
{
  doorState = digitalRead(DOOR_SENSOR_PIN); // read state
  if (doorState == HIGH)
  {
    Serial.println("The door is open");
  }
  else
  {
    Serial.println("The door is closed");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP); // reed switch
  SPIFFS.begin(true);
  WiFiSettings.secure = true;
  WiFiSettings.onPortal = []()
  {
    setup_ota();
  };
  WiFiSettings.onPortalWaitLoop = []()
  {
    ArduinoOTA.handle();
  };
  WiFiSettings.connect();
  Serial.print("Password: ");
  Serial.println(WiFiSettings.password);
  setup_ota();
  firebase_init();
  dht.setup(23); // data pin 2
}

void loop()
{
  ArduinoOTA.handle();

  delay(dht.getMinimumSamplingPeriod());

  redswitch();

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  Serial.print(dht.getStatusString());

  write_data(temperature, humidity);
}
