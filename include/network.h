#ifndef NETW_H
#define NETW_H

#include <Arduino.h>
#include <SPIFFS.h>

void net_process();

void net_setup();

void net_send(const char *name, int32_t value);
void net_send(const char *name, const char * value);

bool get_setting(int pos, uint16_t *offset, uint16_t *values, uint16_t *count);

void set_status(const char* text);

void send_params_to_drive();


#endif