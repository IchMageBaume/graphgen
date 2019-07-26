#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "read_power.h"
#include "svggen.h"

// alway rember, we need to free() power_hist at some point
int main() {
	power_t* power_hist = init_power_over_time("power_state.pht");
	update_power();
	time_t start = time(NULL);

	while (1) {
		sleep(10);
		update_power();
		create_power_graph_svg("graph.svg", power_hist, time(NULL) - start);
	}
}

