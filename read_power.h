#ifndef _READ_POWER_H_INCLUDED_
#define _READ_POWER_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <modbus/modbus.h>

extern int pot_len;

// possibly replace 'struct tm time' with 'time_t time' for more speed and less ram
typedef struct {
	uint16_t watts;
	time_t time;
} power_t;

power_t* init_power_over_time(char* statefile);
void save_power_over_time(char* statefile);
void update_power();

#endif
