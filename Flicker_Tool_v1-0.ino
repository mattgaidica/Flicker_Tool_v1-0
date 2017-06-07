
#include "Statistic.h"
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

//  Serial.begin(19200);
//  Serial.println("Initialised");
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
      count += 2;
    } else {
      count -= 2;
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
    pinMode(flickrPort, OUTPUT);
    digitalWrite(flickrPort,LOW);
    ascDesc();
  }
}

void startupMsg() {
//  oled.clearMsgArea();
  oled.println("Press Green to Start");
  oled.display();
}

// EXPERIMENTAL PROTOCOL
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
  bool runningExp = true;
  bool freqDir = true;
  bool ledState = true;
  int curSample = 0;
  int requireSamples = 2;
  float ascFreqs[] = {0.0, 0.0};
  float descFreqs[] = {0.0, 0.0};
  float startFreq = 25.0;
  float endFreq = 55.0;

  float freqInc = 0.5;
  float curFreq = startFreq;
  int curDelay_ms = 1000 / curFreq;
  int incDelay_ms = 200;
  
  unsigned long lastFreqUpdate_ms = millis();
  unsigned long lastFlicker_ms = millis();
  updateBattery();
  oled.println(curFreq);
  oled.println("Asc, Press Green");
  oled.display();
  while(runningExp) {
    unsigned long cur_ms = millis();
    // make light flicker
    if (cur_ms - lastFlicker_ms >= curDelay_ms / 2) {
      if (ledState == true) {
        digitalWrite(flickrPort,HIGH);
        ledState = false;
      } else {
        digitalWrite(flickrPort,LOW);
        ledState = true;
      }
      lastFlicker_ms = cur_ms;
    }
    // handle inc/dec of freq
    if (cur_ms - lastFreqUpdate_ms >= incDelay_ms) {
      if (freqDir == true) {
        curFreq = curFreq + freqInc;
        if (curFreq > endFreq) {
          curFreq = startFreq;
          delay(1000);
        }
      } else {
        curFreq = curFreq - freqInc;
        if (curFreq < startFreq) {
          curFreq = endFreq;
          delay(1000);
        }
      }
      curDelay_ms = 1000 / curFreq;
      lastFreqUpdate_ms = cur_ms;
    }
    if (!digitalRead(greenBtnPort)) {
      digitalWrite(flickrPort,LOW);
      if (freqDir == true) {
        ascFreqs[curSample] = curFreq;
        freqDir = false;
        updateBattery();
        oled.println(curFreq);
        oled.println("Desc, Press Green");
        oled.display();
        curFreq = endFreq;
      } else {
        descFreqs[curSample] = curFreq;
        freqDir = true;
        curSample++;
        updateBattery();
        oled.println(curFreq);
        oled.println("Asc, Press Green");
        oled.display();
        curFreq = startFreq;
      }
      if (curSample == requireSamples) {
        runningExp = false;
      } else {
        delay(1000);
      }
    }
//    if (!digitalRead(redBtnPort)) {
//      runningExp = false;
//      updateBattery();
//      oled.println("Exiting!");
//      oled.display();
//      delay(500);
//      return;
//    }
  }
  showResults(ascFreqs,descFreqs,requireSamples);
}

void showResults(float ascFreqs[],float descFreqs[],int n) {
  Statistic ascStats;
  ascStats.clear();
  for (int ii = 0; ii < n; ii++) {
    ascStats.add(ascFreqs[ii]);
  }
  updateBattery();
  oled.print("ASC: ");
  oled.print(ascStats.average());
  oled.print(" +/- ");
  oled.println(ascStats.pop_stdev());

  Statistic descStats;
  descStats.clear();
  for (int ii = 0; ii < n; ii++) {
    descStats.add(descFreqs[ii]);
  }
  oled.print("DES: ");
  oled.print(descStats.average());
  oled.print(" +/- ");
  oled.println(descStats.pop_stdev());

  float allAvg[] = {ascStats.average(),descStats.average()};
  Statistic allStats;
  allStats.clear();
  for (int ii = 0; ii < n; ii++) {
    allStats.add(allAvg[ii]);
  }
  oled.print("ALL: ");
  oled.print(allStats.average());
  oled.print(" +/- ");
  oled.println(allStats.pop_stdev());
  
  oled.display();

  bool doLoop = true;
  while (doLoop) {
    if (!digitalRead(redBtnPort)) {
      doLoop = false;
    }
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
