#include "svggen.h"

char* graph_templ = 
"<svg width=\"650\" height=\"200\" xmlns=\"http://www.w3.org/2000/svg\">\n"
"\n"
"<style>\n"
"    .axis {\n"
"        stroke: #222222;\n"
"        stroke-width: 2;\n"
"    }\n"
"\n"
"    .ticks {\n"
"        stroke: #BBBBBB;\n"
"        stroke-width: 2;\n"
"    }\n"
"\n"
"    .timestamps {\n"
"        font: 14px sans-serif;\n"
"    }\n"
"</style>\n"
"\n"
"<!-- graph content -->\n"
"<polyline points=\"%s\" style=\"stroke: #2020FF; stroke-width: 3; fill: #00000000\"/>\n"
"<polygon points=\"630 150, %s70 150\" style=\"fill: #A0A0FF\"/>\n"
"\n"
"<!-- ticks -->\n"
"%s\n"
"\n"
"<!-- axis -->\n"
"<line x1=\"69\" y1=\"151\" x2=\"630\" y2=\"151\" class=\"axis\"/>\n"
"<line x1=\"70\" y1=\"151\" x2=\"70\"  y2=\"20\"  class=\"axis\"/>\n"
"\n"
"<!-- text for ticks -->\n"
"%s\n"
"\n"
"</svg>\n";

void create_power_graph_svg(const char* filename, power_t* power_hist, int secs_back) {
	// show history for 1 day max; maybe make more flexible later
	if (secs_back > 86400) secs_back = 86400;

	struct tm* newest = &(power_hist[POT_LEN - 1].time);
	time_t newest_num = mktime(newest);
	time_t x_0_time = newest_num - secs_back;

	int tick_interval, last_tick_back;
	if (secs_back < 120) {
		tick_interval = 10;
		last_tick_back = newest->tm_sec % 10;
	}
	else if (secs_back < 900) {
		tick_interval = 60;
		last_tick_back = newest->tm_sec;
	}
	else if (secs_back < 7200) {
		tick_interval = 600;
		last_tick_back = newest->tm_sec + (newest->tm_min % 10) * 60;
	}
	else if (secs_back < 28800) {
		tick_interval = 3600;
		last_tick_back = newest->tm_sec + newest->tm_min * 60;
	}
	else {
		tick_interval = 10800;
		last_tick_back = newest->tm_sec + newest->tm_min * 60 + newest->tm_hour % 3;
	}

	char ticks[2048] = { '\0' };
	char tick_text[2048] = { '\0' };
	char buf[128];
	for (time_t i = newest_num - last_tick_back; i >= x_0_time;
		i -= tick_interval)
	{
		memset(buf, '\0', 128);

		int offs_from_start = 630 - (newest_num - i) * 560 / secs_back;
		sprintf(buf,
			"<line x1=\"%d\" y1=\"151\" x2=\"%d\" y2=\"168\" class=\"ticks\"/>\n",
			offs_from_start, offs_from_start);
		strcat(ticks, buf);

		memset(buf, '\0', 128);

		struct tm* i_localtime = localtime(&i);
		if (secs_back < 120) {
			sprintf(buf,
				"<text x=\"%d\" y=\"180\" class=\"timestamps\">%02d:%02d</text>\n",
				offs_from_start - 20, i_localtime->tm_min, i_localtime->tm_sec);
		}
		else {
			sprintf(buf,
				"<text x=\"%d\" y=\"180\" class=\"timestamps\">%d:%02d</text>\n",
				offs_from_start - 20, i_localtime->tm_hour, i_localtime->tm_min);
		}
		strcat(tick_text, buf);
	}

	char* points = malloc(100000);
	off_t points_off = 0;
	int i = POT_LEN - 1;
	int last_x;
	for (; i > 0; i--) {
		time_t power_time = mktime(&(power_hist[i].time));
		int offs_from_start = 630 - (newest_num - power_time) * 560 / secs_back;
		memset(buf, '\0', 128);

		if (power_time >= newest_num - secs_back) {
			sprintf(buf, "%d %d, ", offs_from_start,
				150 - power_hist[i].watts * 130 / 5000);
			memcpy(points + points_off, buf, strlen(buf));
			points_off += strlen(buf);
			if (power_time == newest_num - secs_back) break;
		}
		else {
			// exit if because of timing stuff last_x is 0 so we don't get a floating
			// point exception and exit
			if (last_x == 0) break;
			int watt_diff = power_hist[i].watts - power_hist[i + 1].watts;
			// offs_from_start is x relative to whole svg, so we need to substract the
			// diff from that to x = 0 of the graph
			int watts_at_x_0 = power_hist[i + 1].watts -
				watt_diff * (last_x - (offs_from_start - 70) / ((float)last_x));

			sprintf(buf, "70 %d, ", 150 - watts_at_x_0 * 130 / 5000);
			memcpy(points + points_off, buf, strlen(buf));
			points_off += strlen(buf);
			break;
		}

		last_x = offs_from_start - 70;
	}
	points[points_off] = '\0';
 
	char* svg_content = (char*)malloc(strlen(graph_templ) + points_off * 2 +
		strlen(ticks) + strlen(tick_text));

	// TODO add (optional) compilation and compression of the svg content
	sprintf(svg_content, graph_templ, points, points, ticks, tick_text);

	FILE* svg = fopen(filename, "wb");
	fwrite(svg_content, 1, strlen(svg_content), svg);
	fclose(svg);

	free(svg_content);
	free(points);
}

