#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define LED1 16

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;
const char* mqtt_server = "mqtt.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = MQTT_CLIENT;
const char* mqtt_username = MQTT_USER;
const char* mqtt_password = MQTT_PASSWD;
const char* a = "a";
int prevZero = 0;

WiFiClient espClient;
PubSubClient client(espClient);
char msg[256];

void parse_data(String payload){
  // parse json to object
  const int capacity = JSON_OBJECT_SIZE(2*JSON_OBJECT_SIZE(4));
  StaticJsonDocument<capacity> doc;

  DeserializationError err = deserializeJson(doc, payload);

  if (err){
    Serial.println(err.c_str());
  }
  const char* deviceid = doc["deviceid"]; // "8bfec1ab-4b2b-447e-b478-534121c84b26"

  JsonObject data = doc["data"];
  bool airconOn = data["airconOn"];
  int airconTemp = data["airconTemp"];
  int cap = data["capacity"];
  bool light1 = data["light1"];
  bool light2 = data["light2"];
  bool light3 = data["light3"];
  bool light4 = data["light4"];
  bool light5 = data["light5"];

  // send data to the device
  
  char out[50];
  char len[10];
  sprintf(out, "%d,%d,%d,%d,%d,%d,%d,%d", cap, airconOn, airconTemp, light1,light2,light3,light4,light5);
  sprintf(len, "%02d", strlen(out));
  Serial.write(len, 2);
  delay(250);
  Serial.write(out, strlen(out));

}
void get_data(){
  // get data from the server
    HTTPClient http;  
 
    http.begin("http://api.netpie.io/v2/device/shadow/data");  
    http.addHeader("Authorization",TOKEN);
    int httpCode = http.GET();                                 
 
    if (httpCode > 0) { 
      String payload = http.getString();   
      parse_data(payload);  
    }
 
    http.end();   //Close connection
}


void reconnect() {
    while (!client.connected()) {
        if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
            client.subscribe("@msg/frontendtodevice");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println("try again in 5 seconds");
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // for upcoming message when subscribe
    String message;
    for (int i = 0; i < length; i++) {
        message = message + (char)payload[i];
    }
    get_data();
}

void setup() {
    pinMode(LED1,OUTPUT);
    Serial.begin(115200);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

     
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    String count;
    if (Serial.available()){
      count = Serial.readString();
      // count -1 mean the board request the data
      if (count == "-1"){
        get_data();
        count = "0";
      }
      else if (count == "0"){
        prevZero = 1;
        // auto close
        String data = "{\"data\": {\"count\":" + count +", \"changeId\":" + String(0) +",\"airconOn\":false,\"light1\":false,\"light2\":false,\"light3\":false,\"light4\":false,\"light5\":false}}";
        data.toCharArray(msg, (data.length() + 1));
        client.publish("@shadow/data/update", msg);
        client.publish("@msg/devicetofrontend", a);
      }
      else if (count == "1" && prevZero){
        // auto open
        String data = "{\"data\": {\"count\":" + count +", \"changeId\":" + String(0) +",\"airconOn\":true,\"light1\":true,\"light2\":true,\"light3\":true,\"light4\":true,\"light5\":true}}";
        data.toCharArray(msg, (data.length() + 1));
        client.publish("@shadow/data/update", msg);
        client.publish("@msg/devicetofrontend", a);
        prevZero = 0;
      }else{
        String data = "{\"data\": {\"count\":" + count +", \"changeId\":" + String(0) +"}}";
        data.toCharArray(msg, (data.length() + 1));
        client.publish("@shadow/data/update", msg);
        client.publish("@msg/devicetofrontend", a);
        prevZero = 0;
      }
      }
      delay(500);
    }
