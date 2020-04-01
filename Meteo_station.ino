#include <Event.h>
#include <Timer.h>
#include <Adafruit_BMP085.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include "PMS.h"
#include <WiFi101.h>
#include "APDS9200.h"

#define DHTTYPE DHT22
#define DHTPIN 2

DHT _dht(DHTPIN, DHTTYPE);
PMS _pms(Serial1);
PMS::DATA _data;
Adafruit_BMP085 _bmp;
APDS9200 _light;
WiFiClient _client;
Timer _t;

const unsigned long _myChannelNumber = ;
const String _myWriteAPIKey = "";
char _thingSpeakAddress[] = "api.thingspeak.com";
int _status = WL_IDLE_STATUS;

const unsigned long  _interval = 20 * (60 * 1000); //minutes

char _ssid[] = "ssid";                                                           //wifi name
char _pass[] = "pass";                                                           //password

bool _bmpExist = true;
bool _justStarted = true;

float _pm1sr = 0; //PM 1.0 (ug/m3):
float _pm2g5sr = 0; //PM 2.5 (ug/m3):
float _pm10sr = 0; //PM 10.0 (ug/m3):

float _pressure = 0; //Pa

double _uv = 0;                                                             //uv
int _uvIndex = 0;
float _luminosity = 0;                                                        //light

void setup() {
  // put your setup code here, to run once:


  Serial.begin(9600);
  Serial1.begin(9600);
  if (!_bmp.begin()) {
    _bmpExist = false;
  }

  WiFi.disconnect();
  WiFi.begin(_ssid, _pass);/*change your wifi SSID & PASSSWORD*/
  while (!(WiFi.status() == WL_CONNECTED))
  {
    delay(300);
  }

  int tickEvent = _t.every(_interval, updateThingSpeak, (void*)(-1));
  _dht.begin();
  Wire.begin();
  //  ThingSpeak.begin(_client);
}

float readTemp() {
  float temp = _dht.readTemperature();
  return isnan(temp) ? 0 : temp;
}

float readDTHhumidity() {
  float humidity = _dht.readHumidity();
  return isnan(humidity) ? 0 : humidity;
}

void readStorePressure() {
  if (_bmpExist)
    _pressure = _bmp.readPressure();
  else
    _pressure = 0;
}

void readStoreParticles() {
  while (!_pms.read(_data))
  {

  }
  _pm1sr = _data.PM_AE_UG_1_0;
  _pm2g5sr = _data.PM_AE_UG_2_5;
  _pm10sr = _data.PM_AE_UG_10_0;
}

void readStoreUV() {
  _light.enableUV();
  delay(500);
  _uv = _light.getUV();
}

void readStoreLuminosity() {
  _light.enableLight();
  delay(500);
  _luminosity = _light.getLight();
}

void storeUVindex() {

  if ((_uv >= 0) && (_uv < 275)) {
    _uvIndex = 1;
  } else if ((_uv >= 275) && (_uv < 525)) {
    _uvIndex = 2;
  } else if ((_uv >= 525) && (_uv < 775)) {
    _uvIndex = 3;
  } else if ((_uv >= 775) && (_uv < 1050)) {
    _uvIndex = 4;
  } else if ((_uv >= 1050) && (_uv < 1325)) {
    _uvIndex = 5;
  } else if ((_uv >= 1325) && (_uv < 1675)) {
    _uvIndex = 6;
  } else if ((_uv >= 1675) && (_uv < 1960)) {
    _uvIndex = 7;
  } else if ((_uv >= 1960) && (_uv < 2240)) {
    _uvIndex = 8;
  } else if ((_uv >= 2240) && (_uv < 2520)) {
    _uvIndex = 9;
  } else if ((_uv >= 2520) && (_uv < 2800)) {
    _uvIndex = 10;
  } else if ((_uv >= 2800) && (_uv < 3080)) {
    _uvIndex = 11;
  } else if (_uv >= 3080) {
    _uvIndex = 12;
  } else {
    _uvIndex = 0;
  }
}

int intervalInMS() {
  return _interval * 60 * 1000;
}

void loop() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    // don't continue:
    while (true);
  }

  //attempt to connect to Wifi network:
  if (WiFi.status() != WL_CONNECTED) {
    while ( _status != WL_CONNECTED) {
      _status = WiFi.begin(_ssid, _pass);
      // wait 10 seconds for connection:
      Serial.print("Connecting...");
      delay(10000);
    }
  }
  // put your main code here, to run repeatedly:

  if (_justStarted)
  {
    _justStarted = false;
    thingspeak();
  }
  _t.update();
}

void postThingSpeak(String tsData) {
  if (_client.connect(_thingSpeakAddress, 80)) {
    _client.print("POST /update HTTP/1.1\n");
    _client.print("Host: api.thingspeak.com\n");
    _client.print("Connection: close\n");
    _client.print("X-THINGSPEAKAPIKEY: " + _myWriteAPIKey + "\n");
    _client.print("Content-Type: application/x-www-form-urlencoded\n");
    _client.print("Content-Length: ");
    _client.print(tsData.length());
    _client.print("\n\n");
    _client.print(tsData);

    if (_client.connected()) {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
    }
    _client.stop();
    WiFi.end();
    delay(1000);
    WiFi.begin(_ssid, _pass);
  }
}

void updateThingSpeak(void* context) {
  thingspeak();
}

void thingspeak() {
  //  ThingSpeak.begin(_client);

  readStoreParticles();
  Serial.print("PM 1.0 (ug/m3): ");
  Serial.println(_pm1sr);

  Serial.print("PM 2.5 (ug/m3): ");
  Serial.println(_pm2g5sr);

  Serial.print("PM 10.0 (ug/m3): ");
  Serial.println(_pm10sr);

  readStoreUV();//_uvIndex
  Serial.print("UV raw: ");
  Serial.println(_uv);

  storeUVindex();
  Serial.print("UV index: ");
  Serial.println(_uvIndex);

  readStoreLuminosity();//_luminosity
  Serial.print("Light: ");
  Serial.println(_luminosity);

  readStorePressure(); //_pressure
  Serial.print("Pressure (Pa): ");
  Serial.println(_pressure);

  float temp = readTemp();
  Serial.print("Temperature (*C): ");
  Serial.println(temp);

  float humidity = readDTHhumidity();
  Serial.print("Humidity (%): ");
  Serial.println(humidity);

  // Update ThingSpeak
  if (!_client.connected()) {
    postThingSpeak("field1=" + String(temp) + "&field2=" + String(humidity) + "&field3=" + String(_pm1sr) + "&field4=" + String(_luminosity) + "&field5=" + String(_uvIndex) + "&field6=" + String(_pressure) + "&field7=" + String(_pm2g5sr) + "&field8=" + String(_pm10sr));
  }

  //
  //ThingSpeak.writeField(_myChannelNumber, 1, temp, _myWriteAPIKey);
  //  delay(20000);
  //  ThingSpeak.writeField(_myChannelNumber, 2, humidity, _myWriteAPIKey);
  //  delay(20000);
  //  ThingSpeak.writeField(_myChannelNumber, 3, _pm1sr, _myWriteAPIKey);
  //  delay(20000);
  //  ThingSpeak.writeField(_myChannelNumber, 4, _luminosity, _myWriteAPIKey);
  //  delay(20000);
  //  ThingSpeak.writeField(_myChannelNumber, 5, _uvIndex, _myWriteAPIKey);
  //  delay(20000);
  //  ThingSpeak.writeField(_myChannelNumber, 6, _pressure, _myWriteAPIKey);
  //  delay(20000);
  //  ThingSpeak.writeField(_myChannelNumber, 7, _pm2g5sr, _myWriteAPIKey);
  //  delay(20000);
  //  ThingSpeak.writeField(_myChannelNumber, 8, _pm10sr, _myWriteAPIKey);
  //    ThingSpeak.setField( 1, temp);
  //    ThingSpeak.setField( 2, humidity);
  //    ThingSpeak.setField( 3, _pm1sr);
  //    ThingSpeak.setField( 4, _luminosity);
  //    ThingSpeak.setField( 5, _uvIndex);
  //    ThingSpeak.setField( 6, _pressure);
  //    ThingSpeak.setField( 7, _pm2g5sr);
  //    ThingSpeak.setField( 8, _pm10sr);
  //    ThingSpeak.writeFields(_myChannelNumber, _myWriteAPIKey);
}
