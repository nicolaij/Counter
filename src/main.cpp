#include <Arduino.h>
#include <esp_task_wdt.h>
#include "network.h"
#include "synctime.h"
#include <Bounce2.h>

// IN
#define IN1_PIN 16
#define IN2_PIN 17
#define IN3_PIN 18
#define IN4_PIN 19

// OUT
#define R1_PIN 32
#define R2_PIN 33

Bounce2::Button button_1 = Bounce2::Button();
Bounce2::Button button_2 = Bounce2::Button();
Bounce2::Button button_3 = Bounce2::Button();
Bounce2::Button button_4 = Bounce2::Button();

unsigned long buttons_time[4];

void setup()
{
  Serial.begin(115200);
  Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("ChipRevision %d, Cpu Freq %d MHz, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());

  net_setup();

  init_ee_data();

  // Restart 5 min
  // esp_task_wdt_init(60 * 5, true); //enable panic so ESP32 restarts
  // esp_task_wdt_add(NULL);          //add current thread to WDT watch

  digitalWrite(R1_PIN, 0);
  pinMode(R1_PIN, OUTPUT);
  digitalWrite(R2_PIN, 0);
  pinMode(R2_PIN, OUTPUT);

  button_1.attach(IN1_PIN, INPUT);
  button_1.interval(100);
  button_1.setPressedState(HIGH);

  button_2.attach(IN2_PIN, INPUT);
  button_2.interval(100);
  button_2.setPressedState(HIGH);

  button_3.attach(IN3_PIN, INPUT);
  button_3.interval(100);
  button_3.setPressedState(HIGH);

  button_4.attach(IN4_PIN, INPUT);
  button_4.interval(100);
  button_4.setPressedState(HIGH);
}

void loop()
{
  net_process();

  button_1.update();
  if (button_1.pressed())
  {
    buttons_time[0] = millis();
    printf("IN1!\n");
    digitalWrite(R1_PIN, 1);
    inc_ee_data();
  }

  button_2.update();
  if (button_2.pressed())
  {
    buttons_time[1] = millis();
    printf("IN2!\n");
  }

  button_3.update();
  if (button_3.pressed())
  {
    buttons_time[2] = millis();
    printf("IN3!\n");
  }

  button_4.update();
  if (button_4.pressed())
  {
    buttons_time[3] = millis();
    printf("IN4!\n");
  }

  if (buttons_time[0] > 0)
    if (millis() - buttons_time[0] >= 1000)
    {
      buttons_time[0] = 0;
      digitalWrite(R1_PIN, 0);
    }

    delay(100);
}