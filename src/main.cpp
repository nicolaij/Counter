#include <Arduino.h>
//#include <esp_task_wdt.h>
#include "network.h"
#include "synctime.h"
#include <Bounce2.h>

#ifdef ESP32
// IN
#define IN1_PIN D8
#define IN2_PIN D7
//#define IN3_PIN D6
//#define IN4_PIN D5

// OUT
#define R1_PIN 32
#define R2_PIN 33
#endif

#ifdef ESP8266
// IN
#define IN1_PIN D8
#define IN2_PIN D7

// OUT
#define LEDPIN D4
#define R1_PIN D6
#define R2_PIN D5
#endif
Bounce2::Button button_1 = Bounce2::Button();
Bounce2::Button button_2 = Bounce2::Button();
Bounce2::Button button_3 = Bounce2::Button();
Bounce2::Button button_4 = Bounce2::Button();

unsigned long buttons_time[4] = {0, 0, 0, 0};

void setup()
{
  Serial.begin(115200);
#ifdef ESP32
  Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("ChipRevision %d, Cpu Freq %d MHz, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
#endif
#ifdef ESP8266
  Serial.printf("\n\nSdk version: %s\n", ESP.getSdkVersion());
  Serial.printf("Core Version: %s\n", ESP.getCoreVersion().c_str());
  Serial.printf("Boot Version: %u\n", ESP.getBootVersion());
  Serial.printf("Boot Mode: %u\n", ESP.getBootMode());
  Serial.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Reset reason: %s\n", ESP.getResetReason().c_str());
#endif

  net_setup();

  init_ee_data();

  // Restart 5 min
  // ESP.wdtDisable();
  // ESP.wdtEnable(60 * 5 * 1000);
  // esp_task_wdt_init(60 * 5, true); //enable panic so ESP32 restarts
  // esp_task_wdt_add(NULL);          //add current thread to WDT watch

  digitalWrite(R1_PIN, 0);
  pinMode(R1_PIN, OUTPUT);
  digitalWrite(R2_PIN, 1);
  pinMode(R2_PIN, OUTPUT);

#ifdef LEDPIN
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, 1);
#endif

  button_1.attach(IN1_PIN, INPUT);
  button_1.interval(100);
  button_1.setPressedState(LOW);

  button_2.attach(IN2_PIN, INPUT_PULLUP);
  button_2.interval(100);
  button_2.setPressedState(HIGH);

#ifdef IN3_PIN
  button_3.attach(IN3_PIN, INPUT);
  button_3.interval(100);
  button_3.setPressedState(HIGH);
#endif
#ifdef IN4_PIN
  button_4.attach(IN4_PIN, INPUT);
  button_4.interval(100);
  button_4.setPressedState(HIGH);
#endif
}

void loop()
{
  net_process();

  button_1.update();
  if (button_1.released())
  {
    buttons_time[0] = millis();
    printf("IN1!\n");
    digitalWrite(R1_PIN, 1);
    digitalWrite(R2_PIN, 0);
#ifdef LEDPIN
    digitalWrite(LEDPIN, 0);
#endif
    inc_ee_data();
  }

  button_2.update();
  if (button_2.released())
  {
    buttons_time[1] = millis();
    printf("IN2!\n");
    digitalWrite(R1_PIN, 1);
    digitalWrite(R2_PIN, 0);
#ifdef LEDPIN
    digitalWrite(LEDPIN, 0);
#endif
    inc_ee_data();
  };

#ifdef IN3_PIN
  button_3.update();
  if (button_3.pressed())
  {
    buttons_time[2] = millis();
    printf("IN3!\n");
  }
#endif

#ifdef IN4_PIN
  button_4.update();
  if (button_4.pressed())
  {
    buttons_time[3] = millis();
    printf("IN4!\n");
  }
#endif

  for (int i = 0; i < 4; i++)
  {
    if (buttons_time[i] > 0)
    {
      if (millis() - buttons_time[i] >= 1000)
      {
        buttons_time[i] = 0;
        digitalWrite(R1_PIN, 0);
        digitalWrite(R2_PIN, 1);
#ifdef LEDPIN
        digitalWrite(LEDPIN, 1);
#endif
      }
    }
  }

  delay(10);
}