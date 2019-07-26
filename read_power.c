#include "read_power.h"

int pot_len = 8640;

power_t* power_over_time;

modbus_t* mb;

power_t* init_power_over_time(char* statefile) {
	// TODO get values from statefile

	// initialize all elements but the first one
	// (since that is gonna be overwritten anyway)
	power_over_time = (power_t*)malloc(pot_len * sizeof(power_t));
	time_t now = time(NULL);
	for (int i = 1; i < pot_len; i++) {
		power_over_time[i].watts = 0;
		power_over_time[i].time = now - (pot_len - i - 1) * 10;

	}

	return power_over_time;
}

void save_power_over_time(char* statefile) {
	// TODO
}

void open_new_connection() {
	int times_tried = 0;
	while(1) {
		struct hostent* sma_host = gethostbyname("SMA3005965872");
		char* sma_ip;
		if (sma_host == NULL) {
			printf("An error occured during name resolution; strerror gives: %s\n",
				hstrerror(h_errno));
			puts("I'll just use 192.168.11.21 lul");
			sma_ip = "192.168.11.21";
		}
		else {
			sma_ip = inet_ntoa(*((struct in_addr*)sma_host->h_addr));
		}

		mb = NULL;

		if ((mb = modbus_new_tcp(sma_ip, 502)) == NULL) {
			printf("Error creating modbus connection to %s: %s",
				sma_ip, modbus_strerror(errno));
			goto retry;
		}
		if (modbus_connect(mb) == -1) {
			printf("Error connecting mb: %s\n", modbus_strerror(errno));
			goto retry;
		}

		if (modbus_set_slave(mb, 126) == -1) {
			printf("Error setting slave id: %s\n", modbus_strerror(errno));
			goto retry;
		}

		puts("Connected");
		return;

		retry:
		if (mb != NULL) {
			modbus_close(mb);
			modbus_free(mb);
		}
		// wait longer before retrying to connect every time, but 5m max
		times_tried++;
		int sleep_seconds = times_tried * times_tried;
		sleep(sleep_seconds > 300? 300 : sleep_seconds);
	}
}

// this update+return isn't done in a separate thread even though it can take a while
// since there is no reason to update the svg if we haven't gotten new values
void update_power() {
	// first copy all elements back, since we're gonna add a new one
	// (Linked list or ring buffer kinda thing might be faster but I'm too lazy lul)
	for (int i = 0; i < pot_len - 1; i++) {
		power_over_time[i] = power_over_time[i + 1];
	}

	retry:
	if (mb == NULL) open_new_connection();

	uint16_t power;
	if (modbus_read_registers(mb, 40199, 1, &power) == -1) {
		puts(modbus_strerror(errno));
		modbus_close(mb);
		modbus_free(mb);
		mb = NULL;
		goto retry;
	}

	power_over_time[pot_len - 1].time = time(NULL);
	power_over_time[pot_len - 1].watts = power * 10;
}
