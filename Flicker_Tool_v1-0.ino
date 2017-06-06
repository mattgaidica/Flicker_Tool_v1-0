#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_FeatherOLED.h>

#define VBATPIN A7

Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();

const int photoTransPort = A0; // A0
const int flickrPort = 11;
const int indicatorPort = 13;
const int redBtnPort = 12; // has external pullup
const int greenBtnPort = 5;
int photoTransVal;

int updateBattery_ms = 1000; // 1 s
long updateBattery_ms_last = 0;

int readPhotoTrans_ms = 250; // .25 s

int flickrFade_ms = 10; //
long flickrFade_ms_last = 0;

bool fadeDir = true;
int count = 0;

void setup() {
  pinMode(photoTransPort, INPUT);
  pinMode(flickrPort, OUTPUT);
  pinMode(redBtnPort, INPUT_PULLUP);
  pinMode(greenBtnPort, INPUT_PULLUP);
  pinMode(VBATPIN, INPUT);
  
  oled.init();
  oled.setBatteryVisible(true);
  oled.clearMsgArea();
}

void loop() {
  unsigned long cur_ms = millis();
//  if (cur_ms % readPhotoTrans_us == 0) {
//    photoTransVal = analogRead(photoTransPort);
//    analogWrite(flickrPort, photoTransVal/4);
//  }
  if (cur_ms - flickrFade_ms_last >= flickrFade_ms) {
    analogWrite(flickrPort,count);
    if (fadeDir) {
      count += 1;
    } else {
      count -= 1;
    }
    if (count > 254 || count < 1) {
      fadeDir = !fadeDir;
    }
    flickrFade_ms_last = cur_ms;
  }

  if (cur_ms - updateBattery_ms_last >= updateBattery_ms) {
    updateBattery();
    startupMsg();
    updateBattery_ms_last = cur_ms;
  }

  if (!digitalRead(greenBtnPort)) {
    ascDesc();
  }
}

void startupMsg() {
//  oled.clearMsgArea();
  oled.println("Press Green to Start");
  oled.display();
}

void ascDesc() {
  digitalWrite(flickrPort,false);
  for(int ii=3; ii > 0; ii--) {
    updateBattery();
    oled.print("Starting in ");
    oled.print(ii);
    oled.println("...");
    oled.display();
    delay(1000);
  }
}

float updateBattery() {
  oled.clearDisplay();
  float battery = getBatteryVoltage();
  oled.setBattery(battery);
  oled.renderBattery();
//  oled.display();
  return battery;
}

float getBatteryVoltage() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
}
