#ifndef _READ_POWER_H_INCLUDED_
#define _READ_POWER_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <modbus/modbus.h>

#define POT_LEN 8640

// possibly replace 'struct tm time' with 'time_t time' for more speed and less ram
typedef struct {
	uint16_t watts;
	struct tm time;
} power_t;

power_t* update_power();

#endif
