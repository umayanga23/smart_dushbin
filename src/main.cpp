#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <driver/i2s.h>

// ── OLED ──────────────────────────────────
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Servo ─────────────────────────────────
#define SERVO_PIN 26
Servo lidServo;
#define LID_OPEN   90   // degrees — adjust for your lid
#define LID_CLOSED 0

// ── HC-SR04 ───────────────────────────────
#define TRIG_PIN 18
#define ECHO_PIN 19
#define OPEN_DISTANCE_CM 10   // open lid when closer than this

// ── INMP441 I2S ───────────────────────────
#define I2S_WS   15
#define I2S_SCK  14
#define I2S_SD   32
#define I2S_PORT I2S_NUM_0
#define SAMPLE_BUFFER 512

// ── State ─────────────────────────────────
bool lidOpen = false;
unsigned long lidTimer = 0;
bool voiceTriggered = false;
String statusMsg = "Ready";

// ── Functions ─────────────────────────────

void i2s_install() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = SAMPLE_BUFFER,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
}

void i2s_setpin() {
  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pins);
}

float getSoundLevel() {
  int32_t samples[SAMPLE_BUFFER];
  size_t bytesRead = 0;
  i2s_read(I2S_PORT, samples, sizeof(samples), &bytesRead, 10);
  int count = bytesRead / sizeof(int32_t);
  if (count == 0) return 0;
  long long sum = 0;
  for (int i = 0; i < count; i++) {
    int32_t s = samples[i] >> 14;
    sum += abs(s);
  }
  float avg = (float)(sum / count);
  return constrain((avg / 3000.0f) * 100.0f, 0, 100);
}

long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration / 58;
}

void openLid(String reason) {
  if (!lidOpen) {
    lidServo.write(LID_OPEN);
    lidOpen = true;
    statusMsg = reason;
    Serial.println(">>> LID OPEN — " + reason);
  }
  lidTimer = millis(); // reset timer every trigger
}

void closeLid() {
  lidServo.write(LID_CLOSED);
  lidOpen = false;
  voiceTriggered = false;
  statusMsg = "Ready";
  Serial.println(">>> LID CLOSED");
}

void updateDisplay(long dist, float sound) {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.println("Smart Dustbin");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // Distance
  display.setCursor(0, 14);
  display.print("Dist: ");
  display.print(dist);
  display.println(" cm");

  // Sound level bar
  display.setCursor(0, 26);
  display.print("Sound: ");
  display.print((int)sound);
  display.println("%");
  int barW = (int)(sound * 1.28f);
  display.fillRect(0, 36, barW, 6, SSD1306_WHITE);
  display.drawRect(0, 36, 128, 6, SSD1306_WHITE);

  // Status
  display.drawLine(0, 46, 128, 46, SSD1306_WHITE);
  display.setCursor(0, 50);
  display.setTextSize(1);
  if (lidOpen) {
    display.print("LID: OPEN  ");
    display.print(statusMsg);
  } else {
    display.print("LID: CLOSED");
  }

  display.display();
}

// ── Setup ─────────────────────────────────
void setup() {
  Serial.begin(115200);

  // OLED init
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 25);
  display.setTextSize(1);
  display.println("Smart Dustbin v1.0");
  display.display();
  delay(1500);

  // Servo init
  lidServo.attach(SERVO_PIN);
  lidServo.write(LID_CLOSED);

  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // I2S mic
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  Serial.println("Smart Dustbin Ready!");
}

// ── Loop ──────────────────────────────────
void loop() {
  long dist  = getDistanceCM();
  float sound = getSoundLevel();

  Serial.print("Dist: "); Serial.print(dist);
  Serial.print("cm  Sound: "); Serial.print(sound);
  Serial.println("%");

  // ── Trigger 1: Motion (ultrasonic) ──────
  if (dist > 0 && dist < OPEN_DISTANCE_CM) {
    openLid("Motion");
  }

  // ── Trigger 2: Voice (loud clap/word) ───
  // Threshold 35% — adjust after testing
  if (sound > 35.0f) {
    openLid("Voice");
    voiceTriggered = true;
  }

  // ── Auto close logic ────────────────────
  if (lidOpen) {
    unsigned long elapsed = millis() - lidTimer;

    if (voiceTriggered && elapsed > 3000) {
      // Voice trigger: close after 3 seconds
      closeLid();
    }
    else if (!voiceTriggered && dist >= OPEN_DISTANCE_CM) {
      // Motion trigger: close when person walks away
      closeLid();
    }
  }

  // ── Update OLED ─────────────────────────
  updateDisplay(dist, sound);

  delay(100);
}