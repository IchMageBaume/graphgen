#ifndef _SVGGEN_H_INCLUDED_
#define _SVGGEN_H_INCLUDED_

#include <stdlib.h>

// for power_t and pot_len
#include "read_power.h"

void create_power_graph_svg(const char* filename, power_t* power_hist, int secs_back);

#endif
