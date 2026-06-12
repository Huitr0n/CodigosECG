

#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "BluetoothSerial.h"


const char* SSID     = "iPhone de Emmanuel";
const char* PASSWORD = "jeje1234";

BluetoothSerial SerialBT;

#define ECG_PIN  34


WebSocketsServer wsServer(81);

#define SAMPLE_MS      10
#define BPM_WINDOW     8
#define REFRACTORY_MS  250

long  threshold1  = 0;
long  threshold2  = 0;
long  spk         = 0;
long  npk         = 0;

unsigned long lastSampleTime = 0;
unsigned long lastPeakTime   = 0;
int  peakIntervals[BPM_WINDOW];
int  peakCount    = 0;
float currentBPM  = 0;
uint8_t connectedClient = 255;

unsigned long lastPrint = 0;

String getZone(float bpm) {
  if (bpm < 60)   return "brady";
  if (bpm <= 100) return "normal";
  if (bpm <= 140) return "warning";
  if (bpm <= 180) return "danger";
  return "out";
}

int lowPassFilter(int x) {
  static int y1 = 0, y2 = 0;
  static int buf[13] = {0};
  static int idx = 0;

  buf[idx] = x;
  int x6  = buf[(idx + 13 - 6)  % 13];
  int x12 = buf[(idx + 13 - 12) % 13];
  idx = (idx + 1) % 13;

  int y = (2 * y1 - y2 + x - 2 * x6 + x12) >> 5;
  y2 = y1;
  y1 = y;
  return y;
}

int highPassFilter(int x) {
  static int buf[33] = {0};
  static int idx = 0;
  static long sum = 0;
  static int y1 = 0;

  sum -= buf[idx];
  buf[idx] = x;
  sum += x;
  idx = (idx + 1) % 33;

  int y = x - (sum / 33) - y1;
  y1 = x - (sum / 33);
  return y;
}

int derivativeFilter(int x) {
  static int buf[5] = {0};
  static int idx = 0;

  buf[idx] = x;
  int x1 = buf[(idx + 5 - 1) % 5];
  int x2 = buf[(idx + 5 - 2) % 5];
  int x3 = buf[(idx + 5 - 3) % 5];
  int x4 = buf[(idx + 5 - 4) % 5];
  idx = (idx + 1) % 5;

  return (2*x + x1 - x3 - 2*x4) / 8;
}

#define INT_WIN 30
long movingWindowIntegrator(long x) {
  static long buf[INT_WIN] = {0};
  static int  idx = 0;
  static long sum = 0;

  sum -= buf[idx];
  buf[idx] = x;
  sum += x;
  idx = (idx + 1) % INT_WIN;

  return sum / INT_WIN;
}

bool detectQRS(long intVal, unsigned long now) {
  static long prevVal = 0;
  static bool rising  = false;

  if (intVal > prevVal) {
    rising = true;
  } else if (rising && intVal < prevVal) {
    rising = false;
    long peak = prevVal;

    if (peak > threshold1) {
      unsigned long elapsed = now - lastPeakTime;

      if (elapsed > REFRACTORY_MS) {
        lastPeakTime = now;
        spk = 0.125f * peak + 0.875f * spk;
        threshold1 = npk + 0.25f * (spk - npk);
        threshold2 = 0.5f * threshold1;

        if (elapsed < 3000) {
          peakIntervals[peakCount % BPM_WINDOW] = (int)elapsed;
          peakCount++;

          int n = min(peakCount, BPM_WINDOW);
          long sum = 0;
          for (int i = 0; i < n; i++) sum += peakIntervals[i];
          currentBPM = 60000.0f / ((float)sum / n);

          Serial.printf("★ QRS | BPM: %.1f | Zona: %s | Umbral: %ld\n",
            currentBPM, getZone(currentBPM).c_str(), threshold1);

        
          if (currentBPM > 20 && currentBPM < 300) {
            int bpmInt = (int)currentBPM;
            SerialBT.write(255);               // byte marcador de inicio
            SerialBT.write(bpmInt & 0xFF);     // byte bajo
            SerialBT.write((bpmInt >> 8) & 0xFF); // byte alto
          }
        }
        prevVal = intVal;
        return true;
      }
    } else if (peak > threshold2) {
      npk = 0.125f * peak + 0.875f * npk;
      threshold1 = npk + 0.25f * (spk - npk);
      threshold2 = 0.5f * threshold1;
    }
  }

  prevVal = intVal;
  return false;
}

long panTompkins(int rawADC) {
  int centered   = rawADC - 2048;
  int lp         = lowPassFilter(centered);
  int hp         = highPassFilter(lp);
  int der        = derivativeFilter(hp);
  long sq        = (long)der * (long)der;
  long integrated = movingWindowIntegrator(sq);
  return integrated;
}


void calibrateThresholds() {
  Serial.println("Calibrando umbrales (4 segundos)...");
  long maxVal = 0, sumVal = 0;
  int  count  = 0;

  for (int i = 0; i < 400; i++) {
    int raw = analogRead(ECG_PIN);
    long pt = panTompkins(raw);
    if (pt > maxVal) maxVal = pt;
    sumVal += pt;
    count++;
    delay(10);
  }

  long avgVal = sumVal / count;
  spk        = maxVal;
  npk        = avgVal;
  threshold1 = npk + 0.25f * (spk - npk);
  threshold2 = 0.5f * threshold1;

  Serial.printf("  Max: %ld | Avg: %ld | T1: %ld | T2: %ld\n\n",
    maxVal, avgVal, threshold1, threshold2);
}

void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      connectedClient = num;
      Serial.printf("✓ WS cliente conectado — %s\n",
        wsServer.remoteIP(num).toString().c_str());
      break;
    case WStype_DISCONNECTED:
      if (connectedClient == num) connectedClient = 255;
      Serial.println("✗ WS cliente desconectado");
      break;
    default: break;
  }
}

void sendData(int ecgRaw, int bpm, String zone) {
  if (connectedClient == 255) return;

  StaticJsonDocument<96> doc;
  doc["ecg"] = ecgRaw;
  if (bpm > 0) {
    doc["bpm"]  = bpm;
    doc["zone"] = zone;
  }

  char buf[96];
  serializeJson(doc, buf);
  wsServer.sendTXT(connectedClient, buf);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== CardioLink ESP32 + AD620 ===");
  Serial.println("Modo: WiFi WebSocket + Bluetooth Serial\n");

  
  SerialBT.begin("ESP32_ECG");
  Serial.println("✓ Bluetooth iniciado — nombre: ESP32_ECG");

  Serial.print("Conectando a WiFi");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n✓ WiFi — IP: %s  Puerto WS: 81\n\n",
    WiFi.localIP().toString().c_str());

  wsServer.begin();
  wsServer.onEvent(wsEvent);
  Serial.println("✓ WebSocket iniciado");

  calibrateThresholds();
  Serial.println("✓ Listo — esperando latidos...\n");
}

void loop() {
  wsServer.loop();

  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_MS) {
    lastSampleTime = now;

    int  rawADC   = analogRead(ECG_PIN);
    long ptSignal = panTompkins(rawADC);

    detectQRS(ptSignal, now);

    if (now - lastPrint >= 500) {
      lastPrint = now;
      Serial.printf("RAW: %d | PT: %ld | T1: %ld | BPM: %.1f\n",
        rawADC, ptSignal, threshold1, currentBPM);
    }

    sendData(rawADC, (int)currentBPM, getZone(currentBPM));
  }
}
