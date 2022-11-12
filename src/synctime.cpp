
#include "Arduino.h"
#include "synctime.h"
#include "network.h"

#include <coredecls.h>  // settimeofday_cb()
#include <time.h>
#include "sys/time.h"

#define SDA_PIN D2
#define SCL_PIN D1

#define countof(a) (sizeof(a) / sizeof(a[0]))

#include <Wire.h> // must be included here so that Arduino library object file references work
#include <SPI.h>

RTC_DS1307 rtc;

// long timezone = 3;
// byte daysavetime = 0;

// synctime::synctime() {
//  Start the I2C interface
//   Wire.begin();
// Wire.setClock(100000);
// }

#include "extEEPROM.h"
#define EESIZE kbits_32
extEEPROM myEEPROM(EESIZE, 1, 32, 0x50);

eeprom_block_t ee;

unsigned int num_ee_block;

bool getLocalTime(struct tm * info, uint32_t ms = 5000)
{
    uint32_t start = millis();
    time_t now;
    while((millis()-start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if(info->tm_year > (2016 - 1900)){
            return true;
        }
        delay(10);
    }
    return false;
}

#define PTM(w) \
  Serial.print(" " #w "="); \
  Serial.print(tm->tm_##w);

void printTm(const char* what, const tm* tm) {
  Serial.print(what);
  PTM(isdst);
  PTM(yday);
  PTM(wday);
  PTM(year);
  PTM(mon);
  PTM(mday);
  PTM(hour);
  PTM(min);
  PTM(sec);
}


void synctime::begin()
{

  configTzTime("UTC-3", "pool.ntp.org", "time.windows.com", "time.nist.gov");

  Wire.begin(SDA_PIN, SCL_PIN);

  myEEPROM.begin(extEEPROM::twiClock100kHz, &Wire);

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    return;
  }
  /*
  if (!rtc.isrunning())
  //if (rtc.lostPower())
  {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  */
  printtime();
}

bool synctime::printtime()
{
  DateTime now = rtc.now();

  Serial.print("RTC time: ");

  char buf2[] = "DD.MM.YYYY hh:mm:ss";
  Serial.println(now.toString(buf2));

  return true;
}

bool synctime::sync()
{
  last_sync = false;

  Serial.print("Sync from network: ");
  /*
  get();
  tms.tm_year = 2016 - 1900;
  time_t t = mktime(&tms);
  struct timeval tvnow = {.tv_sec = t};
  settimeofday(&tvnow, NULL);
*/
  if (!getLocalTime(&tms))
  {
    DateTime now = rtc.now();

    tms.tm_year = now.year() - 1900;
    tms.tm_mon = now.month() - 1;
    tms.tm_mday = now.day();
    tms.tm_hour = now.hour();
    tms.tm_min = now.minute();
    tms.tm_sec = now.second();
    time_t t = mktime(&tms);
    // printf("Setting time: %s", asctime(&timeinfo));
    // struct timeval tvnow = {.tv_sec = t - timezone * 60 * 60}; //TODO Костыль
    struct timeval tvnow = {.tv_sec = t};
    settimeofday(&tvnow, NULL);

    Serial.println("fail");
  }
  else
  {
    rtc.adjust(DateTime(tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday, tms.tm_hour, tms.tm_min, tms.tm_sec));
    last_sync = true;
    Serial.println("OK");
    // Serial.printf(" %04d-%02d-%02d %02d:%02d:%02d\n", (tms.tm_year) + 1900, (tms.tm_mon) + 1, tms.tm_mday, tms.tm_hour, tms.tm_min, tms.tm_sec);
  }

  get();
  Serial.printf("ESP time: %04d-%02d-%02d %02d:%02d:%02d\n", (tms.tm_year) + 1900, (tms.tm_mon) + 1, tms.tm_mday, tms.tm_hour, tms.tm_min, tms.tm_sec);

  return true;
}

bool synctime::get()
{
  return getLocalTime(&tms, 0);
}

void printLocalTime()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo, 0))
  {
    Serial.println(F(" Failed to obtain time"));
    return;
  }
  //Serial.println(&timeinfo, " %d %B %Y %H:%M:%S ");
  //Serial.printf(" %d %B %Y %H:%M:%S ", &timeinfo);
  printTm("localtime:", &timeinfo);
}

uint8_t get_ee_data(eeprom_block_t *eep, int num = 0)
{
  return myEEPROM.read(num * sizeof(eeprom_block_t), (uint8_t *)eep, sizeof(eeprom_block_t));
};

uint8_t inc_ee_data()
{
  eeprom_block_t *eep = &ee;
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  date_t c_date;

  c_date.year2k = timeinfo.tm_year - 100; //Количество лет с 1900 года.
  c_date.month = timeinfo.tm_mon + 1;     //Количество месяцев с 1 января.
  c_date.day = timeinfo.tm_mday;

  if ((eep->today.date.day == c_date.day) &&
      (eep->today.date.month == c_date.month) &&
      (eep->today.date.year2k == c_date.year2k))
  {
    eep->today.counter++;
  }
  else
  {
    eep->yesterday.date.day = eep->today.date.day;
    eep->yesterday.date.month = eep->today.date.month;
    eep->yesterday.date.year2k = eep->today.date.year2k;
    eep->yesterday.counter = eep->today.counter;

    eep->today.date.day = c_date.day;
    eep->today.date.month = c_date.month;
    eep->today.date.year2k = c_date.year2k;
    eep->today.counter = 1;
    num_ee_block++;
    if ((unsigned long)(num_ee_block + 1) * sizeof(eeprom_block_t) > EESIZE * 1024UL / 8)
    {
      num_ee_block = 0;
    }
   }

  eep->total++;
  return myEEPROM.write((unsigned long)num_ee_block * sizeof(eeprom_block_t), (byte *)eep, sizeof(eeprom_block_t));
}

int init_ee_data()
{
  int n = 0;
  int tc = 0;
  uint32_t tt = 0;
  // eeprom_block_t eeb;

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  date_t c_date;

  c_date.year2k = timeinfo.tm_year - 100; //Количество лет с 1900 года.
  c_date.month = timeinfo.tm_mon + 1;     //Количество месяцев с 1 января. 0-11
  c_date.day = timeinfo.tm_mday;
  while (get_ee_data(&ee, n) == 0)
  {
    // printf("eeprom: %2d.%02d.%4d - %d\n", ee.today.date.day, ee.today.date.month, ee.today.date.year2k, ee.total);
    //сегодня
    if ((ee.today.date.day == c_date.day) &&
        (ee.today.date.month == c_date.month) &&
        (ee.today.date.year2k == c_date.year2k))
    {
      break;
    }

    //если total < предыдущих
    if (ee.total < tc || ee.time < tt)
    {
      ee.total = tc;
      ee.time = tt;
      break;
    }

    tc = ee.total;
    tt = ee.time;
    n++;
  };

  //если дошли до конца eeprom
  if (n >= EESIZE * 1024UL / 8 / sizeof(eeprom_block_t))
  {
    n = 0;
  }

  //Обновляем если не сегодня
  if ((ee.today.date.day != c_date.day) ||
      (ee.today.date.month != c_date.month) ||
      (ee.today.date.year2k != c_date.year2k))
  {
    ee.today.date.day = c_date.day;
    ee.today.date.month = c_date.month;
    ee.today.date.year2k = c_date.year2k;
    ee.today.counter = 0;

    myEEPROM.write((unsigned long)n * sizeof(eeprom_block_t), (byte *)&ee, sizeof(eeprom_block_t));
  }

  num_ee_block = n;

  return n;
};

void erase_ee_data()
{
  int n = 0;
  uint8_t r = 0;
  memset(&ee, 0xff, sizeof(eeprom_block_t));
  while (n < 4096 / sizeof(eeprom_block_t))
  {
    printf("erase: %3d...", n);
    r = myEEPROM.write((unsigned long)n * sizeof(eeprom_block_t), (byte *)&ee, sizeof(eeprom_block_t));
    printf("done(%d)\n", r);
    n++;
  }

  init_ee_data();
};
