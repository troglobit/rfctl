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

int createSartanoBitstream(const char *pChannelStr, const char *pOn_offStr, int32_t *pTxBitstream, int *repeatCount)
{
	int itemCount = 0;
	int on_offCode;
	int bit;

	on_offCode = atoi(pOn_offStr);	/* ON/OFF 0..1 */
	*repeatCount = SARTANO_REPEAT;

	PRINT("Channel: %s, on_off: %d\n", pChannelStr, on_offCode);

	/* check converted parameters for validity */
	if ((strlen(pChannelStr) != 10) || (on_offCode < 0) || (on_offCode > 1)) {
		fprintf(stderr, "Invalid channel or on/off code\n");
		return 0;
	} else {
		for (bit = 0; bit <= 9; bit++) {
			/* "1" bit */
			if (strncmp(pChannelStr + bit, "1", 1) == 0) {
				pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
				pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
				pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
				pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			}
			/* "0" bit */
			else {
				pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
				pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
				pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
				pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
			}
		}
		if (on_offCode >= 1) {
			/* ON == "10" */
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
		} else {
			/* OFF == "01" */
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		}

		/* add stop/sync bit and command termination char '+' */
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SYNC_PERIOD);
	}

	return itemCount;
}

