/* RF bitbang control tool for NEXA and other RF remote receivers
 *
 * Copyright (C) 2010 Tord Andersson <tord.andersson@endian.se>
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

static int bs(const char *group, const char *chan, const char *onoff, int32_t *bitstream, int *repeat, bool waveman)
{
	int house;
	int channel;
	int enable;
	int code = 0;
	const int unknown = 0x6;
	int bit;
	int bitmask = 0x0001;
	int i = 0;

	*repeat = NEXA_REPEAT;

	house   = (int)((*group) - 65);	/* House 'A'..'P' */
	channel = atoi(chan) - 1;	/* Channel 1..16 */
	enable  = atoi(onoff);		/* ON/OFF 0..1 */

	PRINT("House: %d, channel: %d, on_off: %d\n", house, channel, enable);

	/* check converted parameters for validity */
	if ((house < 0) || (house > 15) ||	// House 'A'..'P'
	    (channel < 0) || (channel > 15) || (enable < 0) || (enable > 1)) {
		fprintf(stderr, "Invalid group (house), channel or on/off code\n");
		return 0;
	}

	/*
	 * b0..b11 code where 'X' will be represented by 1 for simplicity.
	 * b0 will be sent first
	 */
	code  = house;
	code |= (channel << 4);
	if (waveman && enable == 0) {
	} else {
		code |= (unknown << 8);
		code |= (enable << 11);
	}

	/* convert to send cmd bitstream */
	for (bit = 0; bit < 12; bit++) {
		if ((bitmask & code) == 0) {
			/* bit timing might need further refinement */
			/* 340 us high, 1020 us low,  340 us high, 1020 us low */
			bitstream[i++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(NEXA_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(NEXA_LONG_PERIOD);
		} else {	/* add 'X' (floating bit) */
				/* 340 us high, 1020 us low, 1020 us high,  350 us low */
			bitstream[i++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(NEXA_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(NEXA_LONG_PERIOD);
			bitstream[i++] = LIRC_SPACE(NEXA_SHORT_PERIOD);
		}
		bitmask = bitmask << 1;
	}

	/* add stop/sync bit and command termination char '+' */
	bitstream[i++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
	bitstream[i++] = LIRC_SPACE(NEXA_SYNC_PERIOD);

	return i;
}

int nexa_bitstream(const char *house, const char *chan, const char *onoff, int32_t *bitstream, int *repeat)
{
	return bs(house, chan, onoff, bitstream, repeat, false);
}

int waveman_bitstream(const char *house, const char *chan, const char *onoff, int32_t *bitstream, int *repeat)
{
	return bs(house, chan, onoff, bitstream, repeat, true);
}
