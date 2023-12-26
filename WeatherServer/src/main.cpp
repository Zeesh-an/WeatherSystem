#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <HTTPSRedirect.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "WifiCredentials.h"

#define WIFIHOSTNAME ("TheWeatherStation")
#define temperaturecorrection (-1.5)
#define pressurecorrection (60)
#define pollingInterval (3) // 3 mins
#define uploadInterval (15) // 30 mins
#define enableSerial                             // Uncomment to enable Serial.print in loop function

ESP8266WebServer server(80);
Adafruit_BME280 bme;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 18000); // 18000 sec is 5 hrs, i.e +5 GMT

const char *ssid = mySSID;         // Enter SSID in WifiCredentials.h
const char *password = myPASSWORD; // Enter Password in WifiCredentials.h
float temperature = 0;
float humidity = 0;
float pressure = 0;
float heatindex = 0;
unsigned long currentTime0, currentTime1;
unsigned long previousTime0, previousTime1;
unsigned long minutes;

// Google Sheets setup (do not edit)
const char *host = "script.google.com";
const int httpsPort = 443;
const char *fingerprint = "";
// ID from Google Sheets
const char *GScriptId = "AKfycbzU2_2_gPnI9iIh42CC7Wdkl_pw3h6HadZdJNruprBYl54b-HkRwejnqZdRSTWMypMsyQ";
String url = String("/macros/s/") + GScriptId + "/exec";
// Enter command (insert_row or append_row) and your Google Sheets sheet name (Sheet name is Data)
String payload_base = "{\"command\": \"insert_row\", \"sheet_name\": \"Data\", \"values\": ";
String payload_base2 = "";
String payload = "";
String payload2 = "";
HTTPSRedirect *client = nullptr;

//  Function declarations for PlatformIO
float hIndex(float tinc, float RH);
void poll_Sensor();
void upload_Data();
void handle_OnConnect();
void handle_OnDebug();
void handle_NotFound();
void handle_OnForce();
void handle_OnUpdate();
String SendHTML(float temperature, float humidity, float pressure, float heatindex);

void setup()
{
  { // Start Serial
    Serial.begin(115200);
    timeClient.begin();
    delay(100);
    Serial.println("\n\n\n\n\n\n\n\n\n\n");
    delay(500);
    Serial.println("************************************************");
    Serial.println("> Heartbeat!...");
    delay(333);
    Serial.println("  Serial started\n");
  }

  { // Start WiFi and connect to your local wi-fi network
    WiFi.mode(WIFI_STA);
    WiFi.hostname(WIFIHOSTNAME);
    WiFi.begin(ssid, password);
    Serial.print("> Connecting to AP: ");
    Serial.print(ssid);
    Serial.print(" ");
    while (WiFi.status() != WL_CONNECTED) // Wait for connection
    {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("\n  WiFi connected"); // Announce successful connection
  }

  { // Arduino OTA Routines
    ArduinoOTA.setPort(8266);                            // Port defaults to 8266
    ArduinoOTA.setHostname("ESP8266_TheWeatherStation"); // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setPassword("admin");                     // No authentication by default
                                                         // Password can be set with it's md5 value as well
                                                         // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
                                                         // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]()
                       {
                         String type;
                         if (ArduinoOTA.getCommand() == U_FLASH)
                         {
                           type = "sketch";
                         }
                         else
                         { // U_SPIFFS
                           type = "filesystem";
                         }

                         // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                         // Serial.println("Start updating " + type);
                       });

    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
                        Serial.printf("Error[%u]: ", error);
                        if (error == OTA_AUTH_ERROR)
                        {
                          Serial.println("Auth Failed");
                        }
                        else if (error == OTA_BEGIN_ERROR)
                        {
                          Serial.println("Begin Failed");
                        }
                        else if (error == OTA_CONNECT_ERROR)
                        {
                          Serial.println("Connect Failed");
                        }
                        else if (error == OTA_RECEIVE_ERROR)
                        {
                          Serial.println("Receive Failed");
                        }
                        else if (error == OTA_END_ERROR)
                        {
                          Serial.println("End Failed");
                        } });
    ArduinoOTA.begin();
  }

  { // Connect to Google Sheets
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
    Serial.print("\n> Connecting to ");
    Serial.print(host);
    Serial.println("...");
    // Try to connect for a maximum of 5 times
    bool flag = false;
    for (int i = 0; i < 5; i++)
    {
      int retval = client->connect(host, httpsPort);
      if (retval == 1)
      {
        flag = true;
        Serial.println("  Connected");
        break;
      }
      else
        Serial.println("  Connection failed. Retrying...");
    }
    if (!flag)
    {
      Serial.print("  Could not connect to server: ");
      Serial.println(host);
      return;
    }
    delete client;    // delete HTTPSRedirect object
    client = nullptr; // delete HTTPSRedirect object
  }

  { // Start Local Webserver
    server.begin();
    Serial.print("\n> Local server ready at...\n  ");
    Serial.print(WiFi.localIP());
    Serial.println(" on Port 80");
    server.on("/", handle_OnConnect);
    server.on("/debug", handle_OnDebug);
    server.on("/force", upload_Data);
    server.onNotFound(handle_NotFound);
  }

  { // Start Protocols and Sensors
    Wire.begin(D2, D1);
    // bme.begin(0x76);
    Serial.println("\n> Waiting for valid BME280 sensor");
    if (!bme.begin(0x76))
    {
      Serial.println("  No valid BME280 sensor found! Check wiring");
      delay(3000);
    }
    else
    {
      Serial.println("  BME280 sensor initialized");
    }
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X16, // temperature
                    Adafruit_BME280::SAMPLING_X1,  // pressure
                    Adafruit_BME280::SAMPLING_X1,  // humidity
                    Adafruit_BME280::FILTER_OFF);
    bme.setTemperatureCompensation(temperaturecorrection);
    bme.takeForcedMeasurement();
    delay(50);
    temperature = bme.readTemperature();
    pressure = bme.readPressure() / 100.0 + pressurecorrection;
    humidity = bme.readHumidity();
    delay(40);
    heatindex = hIndex(temperature, humidity);
    Serial.println("\n************************************************");
    Serial.println("Initial readings:");
    Serial.print(temperature);
    Serial.print(", ");
    Serial.print(pressure);
    Serial.print(", ");
    Serial.println(humidity);
    Serial.println("\n");
    delay(1000);
    timeClient.update();
    minutes = (timeClient.getMinutes());
    Serial.println(timeClient.getFormattedTime());
  }
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
  timeClient.getMinutes();
  currentTime0 = currentTime1 = millis();
  if (currentTime0 - previousTime0 > 62000)
  {
    previousTime0 = currentTime0;
    minutes += 1;
    if (minutes > 60)
    {
      timeClient.update();
      minutes = (timeClient.getMinutes());
    }
    if (minutes % pollingInterval == 0) // Poll sensor
    {
      poll_Sensor();
      if (minutes % uploadInterval == 0) // Upload data
      {
        upload_Data();
      }
    }
  }
}

float hIndex(float tinc, float RH) // Function for calculating Heat index
{
  float TF = ((tinc * 9 / 5) + 32);
  //  Steadman's formula  ==> can be optimized :: HI = TF * 1.1 - 10.3 + RH * 0.047
  float HI = 0.5 * (TF + 61.0 + ((TF - 68.0) * 1.2) + (RH * 0.094));

  //  Rothfusz regression
  if (HI >= 80)
  {
    const float c1 = -42.379;
    const float c2 = 2.04901523;
    const float c3 = 10.14333127;
    const float c4 = -0.22475541;
    const float c5 = -0.00683783;
    const float c6 = -0.05481717;
    const float c7 = 0.00122874;
    const float c8 = 0.00085282;
    const float c9 = -0.00000199;

    float A = ((c5 * TF) + c2) * TF + c1;
    float B = (((c7 * TF) + c4) * TF + c3) * RH;
    float C = (((c9 * TF) + c8) * TF + c6) * RH * RH;

    HI = A + B + C;

    if ((RH < 13) && (TF <= 112))
    {
      HI -= ((13 - RH) / 4) * sqrt((17 - abs(TF - 95.0)) / 17);
    }
    if ((RH > 87) && (TF < 87))
    {
      HI += ((RH - 85) / 10) * ((87 - TF) / 5);
    }
  }
  return ((HI - 32) * 5 / 9);
}

void poll_Sensor()
{
  bme.takeForcedMeasurement();
  delay(50);
  temperature = bme.readTemperature();
  pressure = bme.readPressure() / 100.0 + pressurecorrection;
  humidity = bme.readHumidity();
  delay(40);
  heatindex = hIndex(temperature, humidity);
#ifdef enableSerial
  Serial.print("  Sensor Polled: ");
  Serial.print(millis());
  Serial.print(" - ");
  Serial.print(temperature);
  Serial.print(", ");
  Serial.print(pressure);
  Serial.print(", ");
  Serial.println(humidity);
#endif
}

void upload_Data()
{
  previousTime1 = currentTime1;
  static bool flag = false;
#ifdef enableSerial
  Serial.print("> Data Uploaded: ");
  Serial.print(millis());
  Serial.print(" ");
#endif
  if (!flag)
  {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr)
  {
    if (!client->connected())
    {
      client->connect(host, httpsPort);
    }
  }
  else
  {
#ifdef enableSerial
    Serial.println("Error creating client object!");
#endif
  }
  // Create json object string to send to Google Sheets
  payload = payload_base + "\"" + temperature + "," + humidity + "," + heatindex + "," + pressure + "\"}";

  // Publish data to Google Sheets
  if (client->POST(url, host, payload))
  {
    Serial.println("Iploadedddddd");
    handle_OnConnect();
  }
  else
  {
// do stuff here if publish was not successful
#ifdef enableSerial
    Serial.println("Error while connecting");
#endif
  }
}

void handle_OnConnect()
{
// #ifdef enableSerial
//   Serial.println("------------------------------");
//   Serial.print("Temperature: ");
//   Serial.print(temperature);
//   Serial.println("°C");
//   Serial.print("Humidity   : ");
//   Serial.print(humidity);
//   Serial.println("%");
//   Serial.print("Feels like : ");
//   Serial.print(heatindex);
//   Serial.println("°C");
//   Serial.print("Pressure   : ");
//   Serial.print(pressure);
//   Serial.println("hPa");
// #endif
  server.send(200, "text/html", SendHTML(temperature, humidity, pressure, heatindex));
}

void handle_OnDebug()
{
  String debugmsg = "<!DOCTYPE html>\n<html>\n<head>\n<title>Debug The Weather Station</title>\n<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  debugmsg += "<link rel='icon' href='data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>&#9925;</text></svg>'>";
  debugmsg += "\n<style>\n html {font-family:Arial, Helvetica, sans-serif; display:block; margin:0px auto; text-align:center; color:#444444;} body {margin:0px; background-color:#000000;}\n .icon {font-size:3em;} h1,h3 {margin:40px auto 5px; color:#ffffff;} h3 {margin: 0px auto 50px; font-size:1em; color:#777777;}\n a:link { text-decoration: none;} a:visited { text-decoration: none;} table {margin:0 auto; width: 19em;}\n .reading {font-weight:300; font-size:3em; text-align:center;} .temp {color:#F29C1F;} .humid {color:#3B97D3;}\n .index {color:#f03333;} .index2 {filter: hue-rotate(90deg);} .press {color:#26B99A;} .press2 {transform: rotate(20deg)}\n .superscript {font-size:1em; font-weight:600; position:relative; top:-15px; text-align:left;}</style>";
  debugmsg += "</head>\n<body>";
  debugmsg += "\n<h2>&#127774; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &#9925; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &#9748;</h2>";
  debugmsg += "\n<h1>The Weather Station</h1>";
  debugmsg += "<div class='reading temp'>" + String(millis()) + "</div>";
  debugmsg += "<div class='reading temp'>" + String(mySSID) + "</div>";
  debugmsg += "<div class='reading temp'>" + String(myPASSWORD) + "</div>";
  server.send(200, "text/html", debugmsg);
}

void handle_OnForce()
{
  // static bool flag = false;
  String debugmsg = "<!DOCTYPE html>\n<html>\n<head>\n<title>The Weather Station</title>\n<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  debugmsg += "<link rel='icon' href='data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>&#9925;</text></svg>'>";
  debugmsg += "\n<style>\n html {font-family:Arial, Helvetica, sans-serif; display:block; margin:0px auto; text-align:center; color:#444444;} body {margin:0px; background-color:#000000;}\n</style>";
  debugmsg += "\n<script>setInterval(loadDoc,500); function loadDoc() { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.body.innerHTML =this.responseText} };\n xhttp.open(\"GET\", \"/\", true); xhttp.send(); }</script>";
  debugmsg += "</head>\n<body>";
  debugmsg += "\n<h2>Polling Sensor...</h2>";
  server.send(200, "text/html", debugmsg);
  bme.takeForcedMeasurement();
  delay(50);
  temperature = bme.readTemperature();
  pressure = bme.readPressure() / 100.0 + pressurecorrection;
  humidity = bme.readHumidity();
  delay(40);
  heatindex = hIndex(temperature, humidity);
//     if (!flag)
//   {
//     client = new HTTPSRedirect(httpsPort);
//     client->setInsecure();
//     flag = true;
//     client->setPrintResponseBody(true);
//     client->setContentTypeHeader("application/json");
//   }
//   if (client != nullptr)
//   {
//     if (!client->connected())
//     {
//       client->connect(host, httpsPort);
//        Serial.println("Connnnected");
//     }
//   }
//   else
//   {
// #ifdef enableSerial
//     Serial.println("Error creating client object!");
// #endif
//   }
  // Create json object string to send to Google Sheets
  payload_base2 = "{\"command\": \"update_cell\", \"sheet_name\": \"LastDefrost\", \"values\": ";
  payload2 = payload_base2 + "\"" + "A1" + "," + humidity + "," + heatindex + "," + pressure + "\"}";

  // Publish data to Google Sheets
  if (client->POST(url, host, payload2))
  {
     Serial.println("Donnnne");
  }
  else
  {
Serial.println("Nope");
Serial.println(temperature);
Serial.println(humidity);
#ifdef enableSerial
    Serial.println("Error while connecting");
#endif
  }
    // String payload_base = "{\"command\": \"update_cell\", \"sheet_name\": \"Data\", \"values\": ";

}

void handle_NotFound()
{
  server.send(404, "text/html", "<!DOCTYPE html><html><head><title>Error 404</title><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>html {font-family:Arial, Helvetica, sans-serif; font-size:2em; display:block; margin:0px auto; text-align:center; color:#ffaa00;} body {margin:0px; background-color:#000000;} h1{margin:20px auto; color:#ffffff; font-size:4em;}</style></head><body><h2>Oops!</h2><h1>&#128561;</h1><h2>Page not found</h2></body></html>");
}

String SendHTML(float temperature, float humidity, float pressure, float heatindex)
{
  String ptr = "<!DOCTYPE html>\n<html>\n<head>\n<title>The Weather Station</title>\n<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  ptr += "<link rel='icon' href='data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>&#9925;</text></svg>'>";
  ptr += "\n<style>\n html {font-family:Arial, Helvetica, sans-serif; display:block; margin:0px auto; text-align:center; color:#444444;} body {margin:0px; background-color:#000000;}\n .icon {font-size:3em;} h1,h3 {margin:40px auto 5px; color:#ffffff;} h3 {margin: 0px auto 50px; font-size:1em; color:#777777;}\n a:link { text-decoration: none;} a:visited { text-decoration: none;} table {margin:0 auto; width: 19em;}\n .reading {font-weight:300; font-size:3em; text-align:right;} .temp {color:#F29C1F;} .humid {color:#3B97D3;}\n .index {color:#f03333;} .index2 {filter: hue-rotate(90deg);} .press {color:#26B99A;} .press2 {transform: rotate(20deg)}\n .superscript {font-size:1em; font-weight:600; position:relative; top:-15px; text-align:left;}</style>";
  ptr += "\n<script>setInterval(loadDoc,180000); function loadDoc() { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { document.body.innerHTML =this.responseText} };\n xhttp.open(\"GET\", \"/\", true); xhttp.send(); }</script>";
  ptr += "</head>\n<body>";
  ptr += "\n<h2>&#127774; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &#9925; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &#9748;</h2>";
  ptr += "\n<h1>The Weather Station</h1>";
  ptr += "\n<a href='https://maps.app.goo.gl/u1voFXGwNocC1sc88'><h3>NUST H-12</h3></a>";
  ptr += "\n<table>\n";
  ptr += "<tr><td><div class='icon'>&#127777;&#65039;</div></td>";
  ptr += "<td><div class='reading temp'>" + String(temperature, 1) + "</div></td>";
  ptr += "<td><div class='temp superscript'>&deg;C</div></td></tr>\n";
  ptr += "<tr><td><div class='icon'>&#128167;</div></td>";
  ptr += "<td><div class='reading humid'>" + String(humidity, 1) + "</div></td>";
  ptr += "<td><div class='humid superscript'>%</div></td></tr>\n";
  ptr += "<tr><td><div class='icon index2'>&#127777;&#65039;</div></td>";
  ptr += "<td><div class='reading index'>" + String(heatindex, 1) + "</div></td>";
  ptr += "<td><div class='index superscript'>&deg;C</div></td></tr>\n";
  ptr += "<tr><td><a href='/force'><div class='icon press2'>&#128347;</div></a></td>";
  ptr += "<td><div class='reading press'>" + String(pressure, 1) + "</div></td>";
  ptr += "<td><div class='press superscript'>hPa</div></td></tr>\n</table>";
  ptr += "\n<div style='display:grid; position:fixed; bottom:0; text-align:center; width:100%'>&copy; MMXXIII Hamza Fawad Anwar<BR>";
  ptr += "\n<span style='font-size:0.7em'>Ver 1.0 - 26 Dec 23</span></div></body>\n</html>";
  return ptr;
}