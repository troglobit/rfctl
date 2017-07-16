/* Control tool for NEXA and other RF remote receivers
 *
 * Copyright (C) 2010, 2012  Tord Andersson <tord.andersson@endian.se>
 * Copyright (C) 2017        Joachim Nilsson <troglobit@gmail.com>
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
 * along with this program; if not, visit the Free Software Foundation
 * website at http://www.gnu.org/licenses/gpl-2.0.html or write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "common.h"
#include "protocol.h"

static int manchester(const char *bitstring, int32_t bitstream[])
{
	int i = 0;
	const char *bit = bitstring;

	while (*bit) {
		if (*bit == '1') {
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		} else {
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
		}

		bit++;
	}

	return i;
}

int sartano_bitstream(const char *chan, const char *onoff, int32_t *bitstream, int *repeat)
{
	int i = 0;
	int enable;

	enable = atoi(onoff);	/* ON/OFF 0..1 */
	*repeat = SARTANO_REPEAT;

	PRINT("Channel: %s, onoff: %d\n", chan, enable);

	/* Validate converted parameters */
	if ((strlen(chan) != 10) || (enable < 0) || (enable > 1)) {
		fprintf(stderr, "Invalid channel or on/off code\n");
		return 0;
	}

	/* Convert channel and onoff to bitstream */
	i  = manchester(chan, &bitstream[i]);
	i += manchester(enable ? "10" : "01", &bitstream[i]);

	/* Add stop/sync bit and command termination char '+' */
	bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
	bitstream[i++] = LIRC_SPACE(SARTANO_SYNC_PERIOD);

	return i;
}

/*
 * Encode the following pattern into a house(I..IV) and channel (1..4):
 *
 *  1000100000 <--> I - 1
 *  1000010000 <--> I - 2
 *  1000001000 <--> I - 3
 *  1000000100 <--> I - 4

 *  0100100000 <--> II - 1
 *  0100010000 <--> II - 2
 *  0100001000 <--> II - 3
 *  0100000100 <--> II - 4

 *  0010100000 <--> III - 1
 *  0010010000 <--> III - 2
 *  0010001000 <--> III - 3
 *  0010000100 <--> III - 4

 *  0001100000 <--> IV - 1
 *  0001010000 <--> IV - 2
 *  0001001000 <--> IV - 3
 *  0001000100 <--> IV - 4
 *
 * https://www.raspberrypi.org/forums/viewtopic.php?t=11159
 */
int conrad_bitstream(const char *house, const char *chan, const char *onoff, int32_t *bitstream, int *repeat)
{
	int i;
	int group;
	int channel;
	char magic[20];

	group   = atoi(house);
	channel = atoi(chan);

	if (group < 1 || group > 4 || channel < 1 || channel > 4) {
		fprintf(stderr, "Invalid group(%d) or channel (%d).\nUse group 1..4, channel 1..4\n", group, channel);
		return 0;
	}

	for (i = 0; i < 4; i++) {
		if (i + 1 == group)
			magic[i] = '1';
		else
			magic[i] = '0';
	}
	for (i = 0; i < 4; i++) {
		if (i + 1 == channel)
			magic[4 + i] = '1';
		else
			magic[4 + i] = '0';
	}
	magic[8] = '0';
	magic[9] = '0';
	magic[10] = 0;

	return sartano_bitstream((const char *)magic, onoff, bitstream, repeat);
}
