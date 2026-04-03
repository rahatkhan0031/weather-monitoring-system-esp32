#include <WiFi.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

/*-----------------------------------------------------------------------
-------------------------Sensor pin definition---------------------------
-------------------------------------------------------------------------*/
#define Rain_SensorPin 14      // Digital Pin (GPIO14)
#define Air_SensorPin 34       // Analog Pin (GPIO34 - ADC1)
#define Temp_Hum_SensorPin 4   // Digital Pin (GPIO4)

/*-------------------------------------------------------------------------
--------------------------Object instantiation-----------------------------
--------------------------------------------------------------------------*/
DHT_Unified dht(Temp_Hum_SensorPin, DHT11);
Adafruit_BMP085 bmp;
WiFiServer server(80);

/*--------------------------------------------------------------------
-------------------------Global variables----------------------------
----------------------------------------------------------------------*/
// SSID & PASSWORD of your Network
const char* ssid = "Semicon Media";
const char* pass = "XXXXXXXXXXXXX";

// Variables to hold weather data
float temperature = 0.0, humidity = 0.0, pressure = 0.0;
int AQI = 0, rainfall = 0;

unsigned long lastSensorUpdate = 0;
unsigned long lastWiFiCheck = 0;

/*---------------------------------------------------------------------
-----------------User Defined Functions--------------------------------
---------------------------------------------------------------------------*/

void wifi_connect() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void wifi_reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected. Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
  }
}

void read_sensor_data() {
  sensors_event_t event;
  
  // Read DHT11
  dht.temperature().getEvent(&event);
  if (!isnan(event.temperature)) temperature = event.temperature;
  
  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) humidity = event.relative_humidity;

  // Read BMP180
  pressure = bmp.readPressure() / 100.0; // Convert Pa to hPa/mbar

  // Read MQ135 (Air Quality)
  // ESP32 ADC is 12-bit (0-4095). 
  int mq135Raw = analogRead(Air_SensorPin);
  float mq135PPM = mq135Raw * (3.3 / 4095.0) * 20.0; // Adjusted for 3.3V logic
  AQI = map(mq135Raw, 0, 4095, 0, 300); 

  // Read Rain Sensor
  rainfall = digitalRead(Rain_SensorPin) == HIGH ? 0 : 1;
}

void send_json_data(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  String json = "{\"temperature\":" + String(temperature) +
                ",\"humidity\":" + String(humidity) +
                ",\"pressure\":" + String(pressure) +
                ",\"aqi\":" + String(AQI) +
                ",\"rainfall\":" + String(rainfall) + "}";
  client.println(json);
}

void send_web_page(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  
  const char* html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>ESP32 Weather Dashboard</title>
<style>
 body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #eceff1; color: #333; text-align: center; padding: 20px; }
 h1 { color: #0277bd; }
 .container { max-width: 800px; margin: auto; }
 .data-card { background: #fff; padding: 20px; border-radius: 12px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); margin: 10px; display: inline-block; width: 40%; min-width: 250px; }
 .graph { background: #fff; padding: 15px; border-radius: 12px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); margin-top: 20px; }
 canvas { width: 100%; height: 350px; }
</style>
</head>
<body>
<h1>ESP32 Weather Station</h1>
<div class='container'>
 <div id='weather'>
    <div class='data-card'><b>Temp:</b> --°C | <b>Hum:</b> --%</div>
 </div>
 <div class='graph'><canvas id='combinedGraph'></canvas></div>
</div>
<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
<script>
 const ctxCombined = document.getElementById('combinedGraph').getContext('2d');
 const combinedChart = new Chart(ctxCombined, {
   type: 'line',
   data: {
     labels: [],
     datasets: [
       { label: 'Temp (°C)', data: [], borderColor: '#ff5722', fill: false, tension: 0.3 },
       { label: 'Humidity (%)', data: [], borderColor: '#03a9f4', fill: false, tension: 0.3 }
     ]
   },
   options: { responsive: true, maintainAspectRatio: false }
 });

 function fetchWeatherData() {
   fetch('/data').then(res => res.json()).then(data => {
     document.getElementById('weather').innerHTML = `
       <div class='data-card'>🌡️ <b>Temp:</b> ${data.temperature}°C <br> 💧 <b>Humidity:</b> ${data.humidity}%</div>
       <div class='data-card'>🌬️ <b>Pressure:</b> ${data.pressure} mbar <br> 😷 <b>AQI:</b> ${data.aqi}</div>
       <div class='data-card'>🌧️ <b>Rainfall:</b> ${data.rainfall ? 'Detected' : 'None'}</div>`;
     
     let time = new Date().toLocaleTimeString();
     combinedChart.data.labels.push(time);
     combinedChart.data.datasets[0].data.push(data.temperature);
     combinedChart.data.datasets[1].data.push(data.humidity);
     if (combinedChart.data.labels.length > 15) {
       combinedChart.data.labels.shift();
       combinedChart.data.datasets.forEach(d => d.data.shift());
     }
     combinedChart.update();
   });
 }
 setInterval(fetchWeatherData, 2000);
</script>
</body>
</html>
)rawliteral";
  client.print(html);
}

void run_local_webserver() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    if (request.indexOf("GET /data") != -1) {
      send_json_data(client);
    } else {
      send_web_page(client);
    }
    client.stop();
  }
}

/*-----------------------------------------------------------------
-----------------------Setup & Loop------------------------------
------------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);
  pinMode(Rain_SensorPin, INPUT);
  pinMode(Air_SensorPin, INPUT);
  
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("BMP085/180 sensor error!");
  }
  
  wifi_connect();
  server.begin();
}

void loop() {
  if (millis() - lastSensorUpdate >= 2000) {
    lastSensorUpdate = millis();
    read_sensor_data();
  }
  
  if (millis() - lastWiFiCheck >= 10000) {
    lastWiFiCheck = millis();
    wifi_reconnect();
  }
  
  run_local_webserver();
}