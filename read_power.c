#include "read_power.h"

int pot_len = 8640;

power_t* power_over_time = NULL;

modbus_t* mb = NULL;

power_t* init_power_over_time(char* statefile, time_t* start) {
	*start = 0;
	FILE* f = NULL;
	if (statefile != NULL) {
		f = fopen(statefile, "rb");
	}

	if (f != NULL) {
		uint8_t len_buf[4];
		fread(len_buf, 1, 4, f);
		pot_len =
			(len_buf[0] << 24) |
			(len_buf[1] << 16) |
			(len_buf[2] <<  8) |
			 len_buf[3];

		power_over_time = (power_t*)malloc(pot_len * sizeof(power_t));

		// buffers get initialized to 0 so it is at least kind of
		// deterministic what happens if the file is too small
		for (int i = 0; i < pot_len; i++) {
			uint8_t watt_buf[2] = { 0 };
			fread(watt_buf, 1, 2, f);
			power_over_time[i].watts =
				(watt_buf[0] << 8) |
				 watt_buf[1];

			uint8_t time_buf[5] = { 0 };
			fread(time_buf, 1, 5, f);
			power_over_time[i].time =
				((uint64_t)(time_buf[0]) << 32) |
				           (time_buf[1]  << 24) |
				           (time_buf[2]  << 16) |
				           (time_buf[3]  <<  8) |
				            time_buf[4];

			if (*start == 0 && power_over_time[i].watts != 0) {
				*start = power_over_time[i].time;
			}
		}
		puts("Restored power history from state file");
	}
	else {
		if (power_over_time == NULL) {
			power_over_time = (power_t*)malloc(pot_len * sizeof(power_t));
		}

		// initialize all elements but the first one
		// (since that is gonna be overwritten anyway)
		time(start);
		for (int i = 1; i < pot_len; i++) {
			power_over_time[i].watts = 0;
			power_over_time[i].time = *start - (pot_len - i - 1) * 10;

		}
	}

	return power_over_time;
}

void save_power_over_time(char* statefile) {
	FILE* f = fopen(statefile, "wb");

	// we could ofc just write pot_len, sizeof(int) but I'd like the format to be
	// machine-independent and thus easier to debug
	uint8_t len_buf[] = {
		(pot_len & (0xFF << 24)) >> 24,
		(pot_len & (0xFF << 16)) >> 16,
		(pot_len & (0xFF <<  8)) >>  8,
		 pot_len & 0xFF
	};
	fwrite(len_buf, 1, 4, f);

	for (int i = 0; i < pot_len; i++) {
		uint16_t watts = power_over_time[i].watts;
		uint8_t watt_buf[] = {
			(watts & (0xFF << 8)) >> 8,
			 watts & 0xFF
		};
		fwrite(watt_buf, 1, 2, f);

		uint64_t time = power_over_time[i].time;
		// 5 bytes will last for a few 10k years, 4 wouldn't work after sometime next
		// century
		uint8_t time_buf[] = {
			(time & ((uint64_t)0xFF << 32)) >> 32,
			(time & ((uint64_t)0xFF << 24)) >> 24,
			(time & ((uint64_t)0xFF << 16)) >> 16,
			(time & ((uint64_t)0xFF <<  8)) >>  8,
			 time & 0xFF
		};
		fwrite(time_buf, 1, 5, f);
	}

	fclose(f);
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
