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

int sartano_bitstream(const char *chan, const char *onoff, int32_t *bitstream, int *repeat)
{
	int i = 0;
	int enable;
	int bit;

	enable = atoi(onoff);	/* ON/OFF 0..1 */
	*repeat = SARTANO_REPEAT;

	PRINT("Channel: %s, onoff: %d\n", chan, enable);

	/* check converted parameters for validity */
	if ((strlen(chan) != 10) || (enable < 0) || (enable > 1)) {
		fprintf(stderr, "Invalid channel or on/off code\n");
		return 0;
	} else {
		for (bit = 0; bit <= 9; bit++) {
			/* "1" bit */
			if (strncmp(chan + bit, "1", 1) == 0) {
				bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
				bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
				bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
				bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			}
			/* "0" bit */
			else {
				bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
				bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
				bitstream[i++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
				bitstream[i++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
			}
		}
		if (enable >= 1) {
			/* ON == "10" */
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
		} else {
			/* OFF == "01" */
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			bitstream[i++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		}

		/* add stop/sync bit and command termination char '+' */
		bitstream[i++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		bitstream[i++] = LIRC_SPACE(SARTANO_SYNC_PERIOD);
	}

	return i;
}

