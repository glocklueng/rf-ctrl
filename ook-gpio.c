/*
 * rf-ctrl - A command-line tool to control 433MHz OOK based devices
 * OOK GPIO-based 433 MHz RF Transeiver driver (ook-gpio.ko kernel module needed)
 *
 * Copyright (C) 2016 Jean-Christophe Rona <jc@rona.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "rf-ctrl.h"

#define HARDWARE_NAME			"OOK GPIO"

#define OOK_GPIO_TIMINGS_PATH		"/sys/devices/platform/ook-gpio.0/timings"
#define OOK_GPIO_FRAME_PATH		"/sys/devices/platform/ook-gpio.0/frame"


static int ook_gpio_init(void) {
	return 0;
}

static void ook_gpio_close(void) {
}

static int ook_gpio_set_timings(struct timing_config *conf) {
	FILE *f_timings;
	int ret;

	f_timings = fopen(OOK_GPIO_TIMINGS_PATH, "w");
	if (!f_timings) {
		fprintf(stderr, "%s: Cannot open timings path \"%s\"\n", HARDWARE_NAME, OOK_GPIO_TIMINGS_PATH);
		return -1;
	}

	ret = fprintf(f_timings, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
			conf->start_bit_h_time, conf->start_bit_l_time,
			conf->end_bit_h_time, conf->end_bit_l_time,
			conf->data_bit0_h_time, conf->data_bit0_l_time,
			conf->data_bit1_h_time, conf->data_bit1_l_time,
			(conf->bit_fmt == RF_BIT_FMT_HL) ? 0 : 1, conf->frame_count);
	if (ret < 0) {
		fprintf(stderr, "%s: Cannot write timings\n", HARDWARE_NAME);
	}

	fclose(f_timings);

	return (ret < 0) ? ret : 0;
}

static int ook_gpio_send_frame(uint8_t *frame_data, uint8_t bit_count) {
	FILE *f_frame;
	uint32_t str_size;
	char *str;
	unsigned int byte_count = (bit_count + 7)/8;
	int ret;
	int i;

	f_frame = fopen(OOK_GPIO_FRAME_PATH, "w");
	if (!f_frame) {
		fprintf(stderr, "%s: Cannot open frame path \"%s\"\n", HARDWARE_NAME, OOK_GPIO_FRAME_PATH);
		return -1;
	}

	/* Size of the string: 1 byte + n bytes + n ',' + '\0' = 4 * (n + 1) */
	str_size = 4 * (byte_count + 1);
	str = (char*) malloc(str_size * sizeof(char));

	sprintf(str, "%u", bit_count);
	for (i = 0; i < byte_count; i++) {
		sprintf(str, "%s,%u", str, frame_data[i]);
	}

	ret = fprintf(f_frame, "%s", str);
	if (ret < 0) {
		fprintf(stderr, "%s: Cannot write frame\n", HARDWARE_NAME);
	}

	free(str);
	fclose(f_frame);

	return (ret < 0) ? ret : 0;
}

static int ook_gpio_send_cmd(struct timing_config *config, uint8_t *frame_data, uint8_t bit_count) {
	int ret = 0;

	ret = ook_gpio_set_timings(config);
	if (ret < 0) {
		fprintf(stderr, "%s timings configuration failed\n", HARDWARE_NAME);
		return ret;
	}

	return ook_gpio_send_frame(frame_data, bit_count);
}

struct rf_hardware_driver ook_gpio_driver = {
	.name = HARDWARE_NAME,
	.cmd_name = "ook-gpio",
	.long_name = "OOK GPIO-based 433 MHz RF Transeiver",
	.init = &ook_gpio_init,
	.close = &ook_gpio_close,
	.send_cmd = &ook_gpio_send_cmd,
};
