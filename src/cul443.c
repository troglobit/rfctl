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

/* Convert generic bitstream format to CUL433 format */
int bitstream2cul443(int32_t *bitstream, int len, int repeat, char *cul)
{
	int i;
	int pulses = 0;
	char tmp[20];

	*cul = '\0';

	strcat(cul, "\r\nX01\r\n");	/* start radio */
	strcat(cul, "E\r\n");	/* empty tx buffer */

	for (i = 0; i < len; i++) {
		sprintf(tmp, "%04X", LIRC_VALUE(bitstream[i]));

		if (LIRC_IS_PULSE(bitstream[i]) == true) {
			strcat(cul, "A");
			strcat(cul, tmp);
			pulses++;
		} else {	/* low */

			strcat(cul, tmp);
			strcat(cul, "\r\n");
		}
	}

	if (pulses > 1) {
		/* number of repetitions */
		sprintf(tmp, "S%02d\r\n", repeat);
		strcat(cul, tmp);

		return strlen(cul);
	}

	cul[0] = '\0';

	return 0;
}
