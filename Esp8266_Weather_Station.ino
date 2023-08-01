#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>

#ifndef STASSID
#define STASSID             "**********" 
#define STAPSK              "**********"
#define GOOGLE_SHEETS_ID    "**********"
#endif

const int tempPin = 4;
const char* googleSheetsID = GOOGLE_SHEETS_ID;
const char* ssid = STASSID;
const char* password = STAPSK;
const char* host = "ADD YOUR OWN HOST NAME";
const char* host2 = "script.google.com";

WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);

int d1 = 0;
int d2 = 0;

void setup(void) {
  sensors.begin();
  Serial.begin(115200);

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (MDNS.begin(host)) {
      // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    Serial.print(F("Open http://"));
    Serial.print(host);
    Serial.println(F(".local"));
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");
  timeClient.begin();
}

void loop(void) {

  MDNS.update();
  timeClient.update();
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  String urlTemp = "https://script.google.com/macros/s/";
  urlTemp += googleSheetsID;
  urlTemp += "/exec?temp=";
  urlTemp += temperatureC;
  if (int(timeClient.getHours()) == 13 && d1 == 0){
    Serial.println(timeClient.getFormattedTime());
    update();
    d1 = 1;
    d2 = 0;
  }
  if (int(timeClient.getHours()) == 22 && d2 == 0){
    Serial.println(timeClient.getFormattedTime());
    update();
    d1 = 0;
    d2 = 1;
  }

  // Check if a client has connected
  WiFiClient client = server.accept();
  if (!client) { return; }
  Serial.println("");
  Serial.println("New client");


  // Wait for data from client to become available
  while (client.connected() && !client.available()) { delay(1); }

  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');

  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush();

  String s;
  if (req == "/") {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    Serial.print(temperatureC);
    Serial.println(" °C");
    if (temperatureC > 25){
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<title>Weaher Station</title><h1 style=\"text-align:center; font-size:50px; color:#BD4F2E;\">Temperature is: ";
    }else if(temperatureC < 10){
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<title>Weaher Station</title><h1 style=\"text-align:center; font-size:50px; color:#646EFF;\">Temperature is: ";
    }else{
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<title>Weaher Station</title><h1 style=\"text-align:center; font-size:50px;\">Temperature is: ";
    }
    s += temperatureC;
    s += " Celsius</h1>\r\n\r\n";
    Serial.println("Sending 200");
    
    // Uncomment the coment under this one to make sure you have done everything right. It will update the Google Shetts when the page is opened//
    //update();

    Serial.println(timeClient.getFormattedTime());
    Serial.println(timeClient.getHours());
    //client.print(urlTemp);
  } else {
    s = "HTTP/1.1 404 Not Found\r\n\r\n";
    Serial.println("Sending 404");
  }
  client.print(s);

  Serial.println("Done with client");
}

void update(){
  WiFiClientSecure client2;
  client2.setInsecure();
  const int httpPort = 443;
  if (!client2.connect(host2, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  String urlTemp = "https://script.google.com/macros/s/";
  urlTemp += googleSheetsID;
  urlTemp += "/exec?temp=";
  urlTemp += temperatureC;
  client2.print(String("GET ") + urlTemp + " HTTP/1.1\r\n" +
      "Host: " + host + "\r\n" +
      "User-Agent: BuildFailureDetectorESP8266\r\n" +
      "Connection: close\r\n\r\n");
}
