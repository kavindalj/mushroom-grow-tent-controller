#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <DHT.h> //DHT and Adafruit Sensor library(https://github.com/adafruit/Adafruit_Sensor)
#include <Ticker.h> //https://github.com/sstaub/Ticker
#include "time.h"

#include "config.h" // import network credentials

#define DHTPIN 27
#define FAN 13
#define MISTMAKER 12
#define LIGHT 14

#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 16200;
const int   daylightOffset_sec = 3600;

// network credentials
// const char* ssid = "SSID";
// const char* password = "PASSWORD";

void send_sensor();
void auto_loop();
void wifi_connect();
bool testWifi(void);
void setupAP(void);
void createWebServer();

Ticker timer;
Ticker looptimer;

char webpage[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controll Dashboard</title>
    <script src="https://kit.fontawesome.com/685df27141.js" crossorigin="anonymous"></script>
    <style>
      body {
        display: flex;
        flex-direction: column;
        align-items: center;
        width: 100%;
        margin-top: 80px;
      }
      .mainContainor {
        display: flex;
        flex-direction: column;
        align-items: center;
        margin-top: 50px;
      }
      h6 {
        margin: 0px;
        padding: 0px;
      }
      h1 {
        margin-bottom: 0px;
      }
      .statusContainor {
        display: flex;
        align-items: center;
        margin-bottom: 50px;
      }
      .statusContainor .componentContainor {
        margin: 20px;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
      }
      .controlContainor {
        display: flex;
        flex-direction: column;
        align-items: flex-start;
      }
      .controlContainor .componentContainor {
        display: flex;
        justify-content: flex-start;
        align-items: center;
        margin-bottom: 35px;
      }
      .controlContainor .componentContainor i {
        margin-right: 20px;
        color: #000000;
      }
      .label {
        display: flex;
        align-items: center;
        width: 200px;
      }
      .statusChange {
        display: flex;
        align-items: center;
        margin-bottom: 10px;
      }
      .statusChange input {
        margin-right: 10px;
        margin-left: 5px;
      }
      .inputContainor {
        display: flex;
        align-items: center;
        margin-bottom: 10px;
      }
      .inputContainor input {
        margin-right: 10px;
        margin-left: 5px;
      }
      button {
        margin-left: 10px;
      }
      .statusChange input{
        margin-right: 20px;
      }
      @media (max-width: 480px) {
        body {
          margin-top: 20px;
        }
        h1 {
          font-size: 1.5em;
        }
        .statusContainor .componentContainor {
          width: 65px;
          margin: 5px;
        }
        .statusContainor {
          margin-bottom: 0px;
        }
        .controlContainor{
          margin-left: 20px;
          margin-right: 20px;
          margin-top: 0px;
        }
        .controlContainor .componentContainor{
          flex-direction: column;
        }
        .label {
          width: 150px;
        }
        .inputContainor {
          display: none;
          flex-direction: column;
          align-items: flex-start;
          margin-left: 20px;
        }
        .inputContainor input {
          margin-top: 5px;
          margin-bottom: 5px;
          margin-left: 0px;
        }
        button {
          margin-left: 0px;
          margin-top: 10px;
        }
        .statusName{
          display: none;
        }
      }
    </style>
  </head>

  <script>

    var connection = new WebSocket('ws://'+location.hostname+':81/');

    var fanControlStatus = 0;   // store selected state from web
    var mistMakerControlStatus = 0;
    var lightControlStatus = 0;
    
    var temp_data = 0;   // store read data from sensor
    var hum_data = 0;
    var fan_data = 0;   // store pin read state ==> on or off now
    var mistMaker_data = 0;
    var light_data = 0;
    
    var fanSelect_data = 0;   // store read state in EEPROM
    var mistMakerSelect_data = 0;
    var lightSelect_data = 0;

    var lowTempLev = 0;   // store input data
    var highTempLev = 0;
    var lowHumLev = 0;
    var highHumLev = 0;
    var lightOnTime = 0;
    var lightOffTime = 0;

    connection.onmessage = function(event){

      var full_data = event.data;
      console.log(full_data);
      var data = JSON.parse(full_data);

      temp_data = data.temp;
      hum_data = data.hum;
      fan_data = data.fan;
      mistMaker_data = data.mist;
      light_data = data.light;

      fanSelect_data = data.fanSelect;
      mistMakerSelect_data = data.mistMakerSelect;
      lightSelect_data = data.lightSelect;

      lowTempLev = data.lowTemp_level;
      highTempLev = data.highTemp_level;
      lowHumLev = data.lowHum_level;
      highHumLev = data.highHum_level;
      lightOnTime = data.lightOn_time;
      lightOffTime = data.lightOff_time;

      var fanSt;
      var mistSt;
      var lightSt;

      if (fan_data == 0){   // change 1 ==> ON & 0 ==> OFF
        fanSt = "off";
      }
      if (fan_data == 1){
        fanSt = "on";
      }
      if (mistMaker_data == 0){
        mistSt = "off";
      }
      if (mistMaker_data == 1){
        mistSt = "on";
      }
      if (light_data == 0){
        lightSt = "off";
      }
      if (light_data == 1){
        lightSt = "on";
      }

      // for auto check check boxes with saved state in EEPROM
      switch(fanSelect_data) {
        case 0:
          document.getElementById('fanOff').checked = true;
          document.getElementById('fanInput').style.display = 'none';
          break;
        case 1:
          document.getElementById('fanOn').checked = true;
          document.getElementById('fanInput').style.display = 'none';
          break;
        case 2:
          document.getElementById('fanAuto').checked = true;
          document.getElementById('fanInput').style.display = 'flex';
          break;
      }
      switch(mistMakerSelect_data) {
        case 0:
          document.getElementById('mistMakerOff').checked = true;
          document.getElementById('mistMakerInput').style.display = 'none';
          break;
        case 1:
          document.getElementById('mistMakerOn').checked = true;
          document.getElementById('mistMakerInput').style.display = 'none';
          break;
        case 2:
          document.getElementById('mistMakerAuto').checked = true;
          document.getElementById('mistMakerInput').style.display = 'flex';
          break;
      }
      switch(lightSelect_data) {
        case 0:
          document.getElementById('lightOff').checked = true;
          document.getElementById('lightInput').style.display = 'none';
          break;
        case 1:
          document.getElementById('lightOn').checked = true;
          document.getElementById('lightInput').style.display = 'none';
          break;
        case 2:
          document.getElementById('lightAuto').checked = true;
          document.getElementById('lightInput').style.display = 'flex';
          break;
      }

      // update status show in web page
      document.getElementById("temp_value").innerHTML = temp_data;
      document.getElementById("hum_value").innerHTML = hum_data;
      document.getElementById("fan_state").innerHTML = fanSt;
      document.getElementById("mistMaker_state").innerHTML = mistSt;
      document.getElementById("light_state").innerHTML = lightSt;

      // change placeholder text
      document.getElementById('LTL').placeholder = lowTempLev;
      document.getElementById('HTL').placeholder = highTempLev;
      document.getElementById('LHL').placeholder = lowHumLev;
      document.getElementById('HHL').placeholder = highHumLev;
      document.getElementById('LONT').placeholder = lightOnTime;
      document.getElementById('LOFFT').placeholder = lightOffTime;
    }

    // click functions
    function fan_on()
    {
      fanControlStatus = 1; 
      console.log("FAN is ON");
      document.getElementById('fanInput').style.display = 'none';
      send_data();
    }

    function fan_off()
    {
      fanControlStatus = 0;
      console.log("FAN is OFF");
      document.getElementById('fanInput').style.display = 'none';
      send_data();
    }

    function fan_auto()
    {
      fanControlStatus = 2;
      console.log("FAN auto");
      document.getElementById('fanInput').style.display = 'flex';
      send_data();
    }

    function mist_on()
    {
      mistMakerControlStatus = 1; 
      console.log("MIST MAKER is ON");
      document.getElementById('mistMakerInput').style.display = 'none';
      send_data();
    }

    function mist_off()
    {
      mistMakerControlStatus = 0;
      console.log("MIST MAKER is OFF");
      document.getElementById('mistMakerInput').style.display = 'none';
      send_data();
    }

    function mist_auto()
    {
      mistMakerControlStatus = 2;
      console.log("MISTMAKER auto");
      document.getElementById('mistMakerInput').style.display = 'flex';
      send_data();
    }

    function light_on()
    {
      lightControlStatus = 1; 
      console.log("LIGHT is ON");
      document.getElementById('lightInput').style.display = 'none';
      send_data();
    }

    function light_off()
    {
      lightControlStatus = 0;
      console.log("LIGHT is OFF");
      document.getElementById('lightInput').style.display = 'none';
      send_data();
    }

    function light_auto()
    {
      lightControlStatus = 2;
      console.log("LIGHT auto");
      document.getElementById('lightInput').style.display = 'flex';
      send_data();
    }

    // get text user input
    function submit(){
      // get values only input isnt empty
      if(document.getElementById("LTL").value){
        lowTempLev = document.getElementById("LTL").value;
      }
      if(document.getElementById("HTL").value){
        highTempLev = document.getElementById("HTL").value;
      }
      if(document.getElementById("LHL").value){
        lowHumLev = document.getElementById("LHL").value;
      }
      if(document.getElementById("HHL").value){
        highHumLev = document.getElementById("HHL").value;
      }
      if(document.getElementById("LONT").value){
        lightOnTime = document.getElementById("LONT").value;
      }
      if(document.getElementById("LOFFT").value){
        lightOffTime = document.getElementById("LOFFT").value;
      }

      console.log(lowTempLev,highTempLev,lowHumLev,highHumLev,lightOnTime,lightOffTime);
      send_data();
    }

    function send_data()
    {
      var full_data = '{"fanControl" :'+fanControlStatus+',"mistMakerControl":'+mistMakerControlStatus+',"lightControl":'+lightControlStatus+',"ltl":'+lowTempLev+',"htl":'+highTempLev+',"lhl":'+lowHumLev+',"hhl":'+highHumLev+',"lont":'+lightOnTime+',"lofft":'+lightOffTime+'}';
      console.log(full_data);
      connection.send(full_data);
    }
  </script>

  <body>
    <h1>Controll Dashboard</h1>
    <h6>created by Kavinda Lakshan Jayarathna</h6>
    <div class="mainContainor">
      <div class="statusContainor">
        <div class="componentContainor">
          <i class="fas fa-thermometer-half"></i>
          <p class="status" id="temp_value">--</p>
          <p class="statusName">Temparature</p>
        </div>
        <div class="componentContainor">
          <i class="fas fa-tint"></i>
          <p class="status" id="hum_value">--</p>
          <p class="statusName">Humidity</p>
        </div>
        <div class="componentContainor">
          <i class="fas fa-fan"></i>
          <p class="status" id="fan_state">--</p>
          <p class="statusName">Fan</p>
        </div>
        <div class="componentContainor">
          <i class="fa-solid fa-shower"></i>
          <p class="status" id="mistMaker_state">--</p>
          <p class="statusName">Mist Maker</p>
        </div>
        <div class="componentContainor">
          <i class="fa-solid fa-lightbulb"></i>
          <p class="status" id="light_state">--</p>
          <p class="statusName">Light</p>
        </div>
      </div>
      <div class="controlContainor">
        <div class="componentContainor">
          <div class="label">
            <i class="fas fa-fan"></i>
            <p>Fan</p>
          </div>
          <div class="rightContainor">
            <div class="statusChange">
              on
              <input type="radio" name="fan" id="fanOn" onclick="fan_on()">
              automatically
              <input type="radio" name="fan" id="fanAuto" onclick="fan_auto()">
              off
              <input type="radio" name="fan" id="fanOff" onclick="fan_off()">
            </div>
            <div class="inputContainor" id="fanInput">
              Low temparature level
              <input type="text" id="LTL">
              High temparature level
              <input type="text" id="HTL">
              <button onclick="submit()">Submit</button>
            </div>
          </div>
        </div>
        <div class="componentContainor">
          <div class="label">
            <i class="fa-solid fa-shower"></i>
            <p>Mist Maker</p>
          </div>
          <div class="rightContainor">
            <div class="statusChange">
              on
              <input type="radio" name="mistMaker" id="mistMakerOn" onclick="mist_on()">
              automatically
              <input type="radio" name="mistMaker" id="mistMakerAuto" onclick="mist_auto()">
              off
              <input type="radio" name="mistMaker" id="mistMakerOff" onclick="mist_off()">
            </div>
            <div class="inputContainor" id="mistMakerInput">
              Low humidity level
              <input type="text" id="LHL">
              High humidity level
              <input type="text" id="HHL">
              <button onclick="submit()">Submit</button>
            </div>
          </div>
        </div>
        <div class="componentContainor">
          <div class="label">
            <i class="fa-solid fa-lightbulb"></i>
            <p>Light</p>
          </div>
          <div class="rightContainor">
            <div class="statusChange">
              on
              <input type="radio" name="light" id="lightOn" onclick="light_on()">
              automatically
              <input type="radio" name="light" id="lightAuto" onclick="light_auto()">
              off
              <input type="radio" name="light" id="lightOff" onclick="light_off()">
            </div>
            <div class="inputContainor" id="lightInput">
              Light on time
              <input type="text" id="LONT">
              Light off time
              <input type="text" id="LOFFT">
              <button onclick="submit()">Submit</button>
            </div>
          </div>
        </div>
      </div>
    </div>  
  </body>
</html>


)=====";

char wifiwebpage[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
<head>
    <title>WiFi Config</title>
    <style>
        html, body {
            height: 100%;
            margin: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            font-family: Arial, sans-serif;
        }
        .container {
            text-align: center;
            border: 1px solid #ccc;
            padding: 20px;
            border-radius: 8px;
            background-color: #f9f9f9;
        }
        .form {
            display: inline-block;
            text-align: left;
            width: 100%;
            max-width: 400px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        input[type="text"],input[type="password"] {
            width: calc(100% - 16px);
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        input[type="submit"] {
            padding: 8px 16px;
            border: none;
            border-radius: 4px;
            background-color: #007bff;
            color: white;
            font-size: 14px;
            cursor: pointer;
            display: block;
            width: 150px;
            margin: 0 auto;
            box-sizing: border-box;
        }
        input[type="submit"]:hover {
            background-color: #0056b3;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        #or{
            margin-bottom: 15px;
            color: #a8a7a7;
            text-align: center;
        }
    </style>
</head>
<script>
    var connection = new WebSocket('ws://'+location.hostname+':81/');

    var HOSTNAME = growcontroller

    function submit() {
        SSID = document.getElementById("ssid").value;
        PASSWD = document.getElementById("password").value;
        HOSTNAME = document.getElementById("HostName").value;
        send_data();
    }
    function send_data() {
        var wifi_data = JSON.stringify({
            ssid: SSID,
            password: PASSWD,
            hostname: HOSTNAME
        });
        connection.send(wifi_data);
    }
</script>
<body>
    <div class="container">
        <h1>Enter Your WiFi Credentials</h1>
        <div class="form">
            <div class="form-group">
                <label for="ssid">WiFi SSID</label>
                <input type="text" id="ssid" name="ssid" length=32 placeholder="Home_WiFi">
            </div>
            <div class="form-group">
                <label for="password">Password</label>
                <input type="password" id="password" name="password" length=64 placeholder="********">
            </div>
            <div id="or">-------   or  -------</div>
            <div class="form-group">
                <label for="HostName">Host Name</label>
                <input type="text" id="HostName" name="HostName" length=32 placeholder="growcontroller">
            </div>
            <input type="submit" value="Change" onclick="submit()">
        </div>
    </div>
</body>
</html>


)=====";

AsyncWebServer server(80); // server port 80
WebSocketsServer websockets(81);

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Page Not found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) 
  {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = websockets.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      String message = String((char*)( payload));    // Get json data
      Serial.println(message);

      
      DynamicJsonDocument doc(200);
      // deserialize the data
      DeserializationError error = deserializeJson(doc, message);
      // parse the parameters we expect to receive (TO-DO: error handling)
      // Test if parsing succeeds.
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      //write data to EEPROM

      int FAN_status = doc["fanControl"];
      int MISTMAKER_status = doc["mistMakerControl"];
      int LIGHT_status = doc["lightControl"];

      if (FAN_status == 2){
        EEPROM.write(3, doc["ltl"]);
        EEPROM.write(4, doc["htl"]);
      }
      if (MISTMAKER_status == 2){
        EEPROM.write(5, doc["lhl"]);
        EEPROM.write(6, doc["hhl"]);
      }
      if (LIGHT_status == 2){
        EEPROM.write(7, doc["lont"]);
        EEPROM.write(8, doc["lofft"]);
      }

      String qsid;
      String qpass;
      String qhost;
      if (doc["password"].as<String>() != "null") {
        qpass = doc["password"].as<String>();
      }
      if (doc["ssid"].as<String>() != "null") {
        qsid = doc["ssid"].as<String>();
      }
      if (doc["hostname"].as<String>() != "null") {
        qhost = doc["hostname"].as<String>();
      }
      Serial.println(qsid);
      Serial.println(qpass);
      Serial.println(qhost);

      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 10; i < 106; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(10 + i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(42 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        wifi_connect();
      }
      if ( qhost.length() > 0 ){
        Serial.println("clearing eeprom");
        for (int i = 106; i < 138; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qhost);
        Serial.println("");

        Serial.println("writing eeprom hostname:");
        for (int i = 0; i < qhost.length(); ++i)
        {
          EEPROM.write(106 + i, qhost[i]);
          Serial.print("Wrote: ");
          Serial.println(qhost[i]);
        }
          wifi_connect();
      }

      Serial.println("write to EEPROM");
      EEPROM.write(0, FAN_status);
      EEPROM.write(1, MISTMAKER_status);
      EEPROM.write(2, LIGHT_status);

      EEPROM.commit();
  }
}

void setup(void)
{
  Serial.begin(115200);

  pinMode(MISTMAKER,OUTPUT);
  pinMode(FAN,OUTPUT);
  pinMode(LIGHT,OUTPUT);

  dht.begin();
  EEPROM.begin(512); //Initialasing EEPROM

  // Connect to Wi-Fi
  wifi_connect();

  server.on("/", [](AsyncWebServerRequest * request)
  { 
    request->send_P(200, "text/html", webpage);
  });

    server.on("/wificonfig", [](AsyncWebServerRequest * request)
  { 
    request->send_P(200, "text/html", wifiwebpage);
  });

  // createWebServer();
  server.onNotFound(notFound);
  server.begin();  // start webserver
  websockets.begin();
  websockets.onEvent(webSocketEvent);

  timer.attach(2,send_sensor);
  looptimer.attach(1,auto_loop);

}


void loop(void)
{
 websockets.loop();
}

// send all data to web
void send_sensor()
{
  // Read humidity
  float h = dht.readHumidity();
  // Read temperature
  float t = dht.readTemperature();

  // Read pins
  int fan_state = digitalRead(FAN);
  int mistMaker_state = digitalRead(MISTMAKER);
  int light_state = digitalRead(LIGHT);

  // Read EEProm for loard selcted state and values
  int fan_selected = EEPROM.read(0);
  int mistMaker_selected = EEPROM.read(1);
  int light_selected = EEPROM.read(2);

  int lowTemp_level = EEPROM.read(3);
  int highTemp_level = EEPROM.read(4);
  int lowHum_level = EEPROM.read(5);
  int highHum_level = EEPROM.read(6);
  int lightOn_time = EEPROM.read(7);
  int lightOff_time = EEPROM.read(8);
  
  if (isnan(h) || isnan(t) ) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
    if (isnan(fan_state) || isnan(mistMaker_state) || isnan(light_state) ) {
    Serial.println(F("Failed to read from pins!"));
    return;
  }

  // Make data into json format ==> JSON_Data = {"temp":t,"hum":h}
  String JSON_Data = "{\"temp\":";
         JSON_Data += t;
         JSON_Data += ",\"hum\":";
         JSON_Data += h;
         JSON_Data += ",\"fan\":";
         JSON_Data += fan_state;
         JSON_Data += ",\"mist\":";
         JSON_Data += mistMaker_state;
         JSON_Data += ",\"light\":";
         JSON_Data += light_state;
         JSON_Data += ",\"fanSelect\":";
         JSON_Data += fan_selected;
         JSON_Data += ",\"mistMakerSelect\":";
         JSON_Data += mistMaker_selected;
         JSON_Data += ",\"lightSelect\":";
         JSON_Data += light_selected;
         JSON_Data += ",\"lowTemp_level\":";
         JSON_Data += lowTemp_level;
         JSON_Data += ",\"highTemp_level\":";
         JSON_Data += highTemp_level;
         JSON_Data += ",\"lowHum_level\":";
         JSON_Data += lowHum_level;
         JSON_Data += ",\"highHum_level\":";
         JSON_Data += highHum_level;
         JSON_Data += ",\"lightOn_time\":";
         JSON_Data += lightOn_time;
         JSON_Data += ",\"lightOff_time\":";
         JSON_Data += lightOff_time;
         JSON_Data += "}";
   Serial.println(JSON_Data);     
  websockets.broadcastTXT(JSON_Data);
}

// read EEPROM and perform actions related to that data
void auto_loop(){
  // Read EEProm for loard selcted state and values
  int fan_selected = EEPROM.read(0);
  int mistMaker_selected = EEPROM.read(1);
  int light_selected = EEPROM.read(2);

  if (fan_selected != 2){
    digitalWrite(FAN,fan_selected);
  }else{
    int lowTemp_level = EEPROM.read(3);
    int highTemp_level = EEPROM.read(4);
    float t = dht.readTemperature();
    if ( lowTemp_level >= t ){
      digitalWrite(FAN,0);
    }
    if ( highTemp_level <= t ){
      digitalWrite(FAN,1);
    }
  }

  if (mistMaker_selected != 2){
    digitalWrite(MISTMAKER,mistMaker_selected);
  }else{
    int lowHum_level = EEPROM.read(5);
    int highHum_level = EEPROM.read(6);
    float h = dht.readHumidity();
    if ( lowHum_level >= h ){
      digitalWrite(MISTMAKER,1);
    }
    if ( highHum_level <= h ){
      digitalWrite(MISTMAKER,0);
    }
  }

  if (light_selected != 2){
    digitalWrite(LIGHT,light_selected);
  }else{
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }

    int lightOn_time = EEPROM.read(7);
    int lightOff_time = EEPROM.read(8);

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    int currentHour = timeinfo.tm_hour;
    if (lightOn_time <= currentHour && lightOff_time > currentHour){
      digitalWrite(LIGHT,1);
    }else{
      digitalWrite(LIGHT,0);
    }
  }
}

void wifi_connect(){

  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();

  // Read eeprom for ssid, password and host name
  Serial.println("Reading EEPROM wifi data");

  String esid;
  for (int i = 10; i < 42; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);

  String epass = "";
  for (int i = 42; i < 106; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  
  String ehost = "";
  for (int i = 106; i < 138; ++i)
  {
    ehost += char(EEPROM.read(i));
  }
  Serial.print("HOST: ");
  Serial.println(ehost);

  Serial.println("Reading EEPROM pass");

  //connect to wifi
  WiFi.begin(esid.c_str(), epass.c_str());

  //if wifi didn't connect open AP
  if (testWifi())
  {
    Serial.println("Succesfully Connected!!!");
    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Initialize mDNS
    if (!MDNS.begin(ehost.c_str())) { // hostname
      Serial.println("Error starting mDNS");
      return;
    }
    Serial.println("mDNS started");
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    setupAP(); // Setup HotSpot
    // Print ESP soft IP Address
    Serial.println(WiFi.softAPIP());

    // Initialize mDNS
    if (!MDNS.begin("growcontroller")) { // hostname on AP mode
      Serial.println("Error starting mDNS");
      return;
    }
    Serial.println("mDNS started");
  }
}

bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      delay(10);
    }
  }
  delay(100);

  WiFi.softAP("growcontroller", "KLJ_Creations");
  Serial.println("opensoftAP");
}