#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#if defined(ESP32)
#define RECEIVE_ATTR IRAM_ATTR
// esp32! Receiver CC_GDO2 on GPIO pin 4. Transmit CC_GDO0 on GPIO pin 2.
int CC_GDO2 = 4;
int CC_GDO0 = 2;

#elif defined(ESP8266)
#define RECEIVE_ATTR IRAM_ATTR
// esp8266! Receiver CC_GDO2 on pin 4 = D2. Transmit CC_GDO0 on pin  5 = D1.
int CC_GDO2 = 4;
int CC_GDO0 = 5;

#else
#define RECEIVE_ATTR
// Arduino! Receiver CC_GDO2 on interrupt 0 => that is pin #2 transmit CC_GDO0
// on 6
int CC_GDO2 = 0;
int CC_GDO0 = 6;
#endif

uint8_t ReciveInteruptPin = digitalPinToInterrupt(CC_GDO2);

#define samplesize 2000

const int minsample = 30;
int samplecount;
static unsigned long lastTime = 0;
unsigned long sample[samplesize];
unsigned long samplesmooth[samplesize];

int error_toleranz = 200;

void setupRadio() {
  if (ELECHOUSE_cc1101.getCC1101()) {  // Check the CC1101 Spi connection.
    Serial.println("Connection OK");
  } else {
    Serial.println("Connection Error");
  }
}

bool checkReceived(void) {
  delay(1);
  if (samplecount >= minsample && micros() - lastTime > 100000) {
    detachInterrupt(ReciveInteruptPin);
    return 1;
  } else {
    return 0;
  }
}

void RECEIVE_ATTR receiver() {
  const long time = micros();
  const unsigned int duration = time - lastTime;

  if (duration > 100000) {
    samplecount = 0;
  }

  if (duration >= 100) {
    sample[samplecount++] = duration;
  }

  if (samplecount >= samplesize) {
    detachInterrupt(ReciveInteruptPin);
    checkReceived();
  }

  lastTime = time;
}

void enableReceive() {
  pinMode(CC_GDO2, INPUT);
  ELECHOUSE_cc1101.SetRx();
  samplecount = 0;
  attachInterrupt(ReciveInteruptPin, receiver, CHANGE);
}

void setRadioToRecive(byte modulation, float setrxbw, float frequency,
                      float deviation, float datarate) {
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setPA(0);
  ELECHOUSE_cc1101.setSyncMode(0);
  ELECHOUSE_cc1101.setPktFormat(3);
  ELECHOUSE_cc1101.setModulation(modulation);
  ELECHOUSE_cc1101.setRxBW(setrxbw);
  ELECHOUSE_cc1101.setMHZ(frequency);
  ELECHOUSE_cc1101.setDeviation(deviation);
  ELECHOUSE_cc1101.setDRate(datarate);
  enableReceive();
}

void printReceived() {
  Serial.print("Count=");
  Serial.println(samplecount);
  //   logs.println("<br>");
  //   logs.println("<br>");
  //   logs.println('\n');
  //   logs.println('\n');
  //   logs.print("Count=");
  //   logs.println(samplecount);
  //   logs.println("<br>");
  //   logs.println('\n');

  for (int i = 1; i < samplecount; i++) {
    Serial.print(sample[i]);
    Serial.print(",");
    // logs.print(sample[i]);
    // logs.print(",");
  }
  Serial.println();
  Serial.println();
  //   logs.println("<br>");
  //   logs.println("<br>");
  //   logs.println('\n');
  //   logs.println('\n');
}

void signalanalyse() {
#define signalstorage 10

  int signalanz = 0;
  int timingdelay[signalstorage];
  float pulse[signalstorage];
  long signaltimings[signalstorage * 2];
  int signaltimingscount[signalstorage];
  long signaltimingssum[signalstorage];
  long signalsum = 0;

  for (int i = 0; i < signalstorage; i++) {
    signaltimings[i * 2] = 100000;
    signaltimings[i * 2 + 1] = 0;
    signaltimingscount[i] = 0;
    signaltimingssum[i] = 0;
  }
  for (int i = 1; i < samplecount; i++) {
    signalsum += sample[i];
  }

  for (int p = 0; p < signalstorage; p++) {
    for (int i = 1; i < samplecount; i++) {
      if (p == 0) {
        if (sample[i] < signaltimings[p * 2]) {
          signaltimings[p * 2] = sample[i];
        }
      } else {
        if (sample[i] < signaltimings[p * 2] &&
            sample[i] > signaltimings[p * 2 - 1]) {
          signaltimings[p * 2] = sample[i];
        }
      }
    }

    for (int i = 1; i < samplecount; i++) {
      if (sample[i] < signaltimings[p * 2] + error_toleranz &&
          sample[i] > signaltimings[p * 2 + 1]) {
        signaltimings[p * 2 + 1] = sample[i];
      }
    }

    for (int i = 1; i < samplecount; i++) {
      if (sample[i] >= signaltimings[p * 2] &&
          sample[i] <= signaltimings[p * 2 + 1]) {
        signaltimingscount[p]++;
        signaltimingssum[p] += sample[i];
      }
    }
  }

  int firstsample = signaltimings[0];

  signalanz = signalstorage;
  for (int i = 0; i < signalstorage; i++) {
    if (signaltimingscount[i] == 0) {
      signalanz = i;
      i = signalstorage;
    }
  }

  for (int s = 1; s < signalanz; s++) {
    for (int i = 0; i < signalanz - s; i++) {
      if (signaltimingscount[i] < signaltimingscount[i + 1]) {
        int temp1 = signaltimings[i * 2];
        int temp2 = signaltimings[i * 2 + 1];
        int temp3 = signaltimingssum[i];
        int temp4 = signaltimingscount[i];
        signaltimings[i * 2] = signaltimings[(i + 1) * 2];
        signaltimings[i * 2 + 1] = signaltimings[(i + 1) * 2 + 1];
        signaltimingssum[i] = signaltimingssum[i + 1];
        signaltimingscount[i] = signaltimingscount[i + 1];
        signaltimings[(i + 1) * 2] = temp1;
        signaltimings[(i + 1) * 2 + 1] = temp2;
        signaltimingssum[i + 1] = temp3;
        signaltimingscount[i + 1] = temp4;
      }
    }
  }

  for (int i = 0; i < signalanz; i++) {
    timingdelay[i] = signaltimingssum[i] / signaltimingscount[i];
  }

  if (firstsample == sample[1] and firstsample < timingdelay[0]) {
    sample[1] = timingdelay[0];
  }

  bool lastbin = 0;
  for (int i = 1; i < samplecount; i++) {
    float r = (float)sample[i] / timingdelay[0];
    int calculate = r;
    r = r - calculate;
    r *= 10;
    if (r >= 5) {
      calculate += 1;
    }
    if (calculate > 0) {
      if (lastbin == 0) {
        lastbin = 1;
      } else {
        lastbin = 0;
      }
      if (lastbin == 0 && calculate > 8) {
        Serial.print(" [Pause: ");
        Serial.print(sample[i]);
        Serial.println(" samples]");
        // logs.print(" [Pause: ");
        // logs.print(sample[i]);
        // logs.println(" samples]");
      } else {
        for (int b = 0; b < calculate; b++) {
          Serial.print(lastbin);
          //   logs.print(lastbin);
        }
      }
    }
  }
  //   logs.println("<br>");
  //   logs.println("<br>");
  //   logs.println('\n');
  //   logs.println('\n');
  Serial.println();
  Serial.print("Samples/Symbol: ");
  Serial.println(timingdelay[0]);
  Serial.println();
  //   logs.println('\n');
  //   logs.print("Samples/Symbol: ");
  //   logs.println(timingdelay[0]);
  //   logs.println("<br>");
  //   logs.println('\n');

  int smoothcount = 0;
  for (int i = 1; i < samplecount; i++) {
    float r = (float)sample[i] / timingdelay[0];
    int calculate = r;
    r = r - calculate;
    r *= 10;
    if (r >= 5) {
      calculate += 1;
    }
    if (calculate > 0) {
      samplesmooth[smoothcount] = calculate * timingdelay[0];
      smoothcount++;
    }
  }
  Serial.println("Rawdata corrected:");
  Serial.print("Count=");
  Serial.println(smoothcount + 1);
  //   logs.print("Count=");
  //   logs.println(smoothcount+1);
  //   logs.println("<br>");
  //   logs.println('\n');
  //   logs.println("Rawdata corrected:");
  for (int i = 0; i < smoothcount; i++) {
    Serial.print(samplesmooth[i]);
    Serial.print(",");
    // transmit_push[i] = samplesmooth[i]; // store for transmit
    // logs.print(samplesmooth[i]);
    // logs.print(",");
  }
  Serial.println();
  Serial.println();
  //   logs.println("<br>");
  //   logs.println('\n');
  return;
}

// rav recive loop action
void rawRecive() {
  Serial.println("recive");
  if (checkReceived()) {
    printReceived();
    // signalanalyse();
    enableReceive();
  }
}