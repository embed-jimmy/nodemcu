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
int count = 0;
const char* a = "a";

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];

void parse_data(String payload){
  
  const int capacity = JSON_OBJECT_SIZE(2*JSON_OBJECT_SIZE(4));
  StaticJsonDocument<capacity> doc;

  DeserializationError err = deserializeJson(doc, payload);

  if (err){
    Serial.println(err.c_str());
  }
  const char* deviceid = doc["deviceid"]; // "8bfec1ab-4b2b-447e-b478-534121c84b26"
  Serial.println(deviceid);
  JsonObject data = doc["data"];
  bool airconOn = data["airconOn"];
  int airconTemp = data["airconTemp"];
  int cap = data["capacity"];
  bool light1 = data["light1"];
  bool light2 = data["light2"];
  bool light3 = data["light3"];
  bool light4 = data["light4"];
  bool light5 = data["light5"];
  Serial.println(airconOn);
  Serial.println(airconTemp);
  Serial.println(cap);
  Serial.println(light1);
  Serial.println(light2);
  Serial.println(light3);
  Serial.println(light4);
  Serial.println(light5); 

}
void get_data(){
    HTTPClient http;  
 
    http.begin("http://api.netpie.io/v2/device/shadow/data");  
    http.addHeader("Authorization",TOKEN);
    int httpCode = http.GET();                                 
 
    if (httpCode > 0) { 
 
      String payload = http.getString();   
      parse_data(payload);
      Serial.println(payload);           
 
    }
 
    http.end();   //Close connection
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection…");
        if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
            Serial.println("connected");
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
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String message;
    for (int i = 0; i < length; i++) {
        message = message + (char)payload[i];
    }
    Serial.println(message);
    get_data();
    
    
//    if(String(topic) == "@msg/led") {
//        if(message == "on"){
//            digitalWrite(LED1,0);
//            client.publish("@shadow/data/update", "{\"data\" : {\"led\" : \"on\"}}");
//            Serial.println("LED on");
//        }
//        else if (message == "off"){
//            digitalWrite(LED1,1);
//            client.publish("@shadow/data/update", "{\"data\" : {\"led\" : \"off\"}}");
//            Serial.println("LED off");
//        }
//    }
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
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    
 
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    String data = "{\"data\": {\"count\":" + String(count) +", \"changeId\":" + String(0) +"}}";
//    String changeID = "{\"data\": {\"changeId\":" + String(0) +"}}";
    Serial.println(data);
//    Serial.println(changeID);
    data.toCharArray(msg, (data.length() + 1));
    client.publish("@shadow/data/update", msg);
//    data.toCharArray(msg, (changeID.length() + 1));
//    client.publish("@shadow/data/update", msg);
    client.publish("@msg/devicetofrontend", a);
    count = count + 1;
    delay(2000);
}
