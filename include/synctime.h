/*

*/

#ifndef SYNCTIME_H_
#define SYNCTIME_H_

//#include <time.h>
//#include <sys/time.h>
#include <RTClib.h>

class synctime;

class synctime {
  public:
    void begin();

    bool sync();
    bool get();

    bool printtime();

    struct tm tms;

    bool h12 = false;
    bool Century = false;
    bool PM = false;;

    bool last_sync = false;
};

void printLocalTime();

struct date_t
{
  uint16_t year2k : 7;
  uint16_t month : 4;
  uint16_t day : 5;
};

struct eeprom_data_t
{
  date_t date;
  uint16_t counter;
};

struct eeprom_block_t
{
  int32_t total;
  eeprom_data_t today;
  eeprom_data_t yesterday;
  uint32_t time;
  uint32_t t[4];
};

uint8_t get_ee_data(eeprom_block_t * buf, int num);

uint8_t inc_ee_data();
int init_ee_data();
void erase_ee_data();

#endif /* ASYNCTCP_H_ */
