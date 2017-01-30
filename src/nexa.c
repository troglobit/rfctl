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

#define NEXA_SHORT_PERIOD 340	/* microseconds */
#define NEXA_LONG_PERIOD  1020	/* microseconds */
#define NEXA_SYNC_PERIOD  (32 * NEXA_SHORT_PERIOD)	/* between frames */
#define NEXA_REPEAT 4

int createNexaBitstream(const char *pHouseStr, const char *pChannelStr,
			const char *pOn_offStr, bool waveman, int32_t *pTxBitstream, int *repeatCount)
{
	int houseCode;
	int channelCode;
	int on_offCode;
	int txCode = 0;
	const int unknownCode = 0x6;
	int bit;
	int bitmask = 0x0001;
	int itemCount = 0;

	*repeatCount = NEXA_REPEAT;

	houseCode = (int)((*pHouseStr) - 65);	/* House 'A'..'P' */
	channelCode = atoi(pChannelStr) - 1;	/* Channel 1..16 */
	on_offCode = atoi(pOn_offStr);	/* ON/OFF 0..1 */

	PRINT("House: %d, channel: %d, on_off: %d\n", houseCode, channelCode, on_offCode);

	/* check converted parameters for validity */
	if ((houseCode < 0) || (houseCode > 15) ||	// House 'A'..'P'
	    (channelCode < 0) || (channelCode > 15) || (on_offCode < 0) || (on_offCode > 1)) {
		fprintf(stderr, "Invalid group (house), channel or on/off code\n");
		return 0;
	}

	/* b0..b11 txCode where 'X' will be represented by 1 for simplicity.
	   b0 will be sent first */
	txCode = houseCode;
	txCode |= (channelCode << 4);
	if (waveman && on_offCode == 0) {
	} else {
		txCode |= (unknownCode << 8);
		txCode |= (on_offCode << 11);
	}

	/* convert to send cmd bitstream */
	for (bit = 0; bit < 12; bit++) {
		if ((bitmask & txCode) == 0) {
			/* bit timing might need further refinement */
			/* 340 us high, 1020 us low,  340 us high, 1020 us low */
			pTxBitstream[itemCount++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(NEXA_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(NEXA_LONG_PERIOD);
		} else {	/* add 'X' (floating bit) */
				/* 340 us high, 1020 us low, 1020 us high,  350 us low */
			pTxBitstream[itemCount++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(NEXA_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(NEXA_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(NEXA_SHORT_PERIOD);
		}
		bitmask = bitmask << 1;
	}

	/* add stop/sync bit and command termination char '+' */
	pTxBitstream[itemCount++] = LIRC_PULSE(NEXA_SHORT_PERIOD);
	pTxBitstream[itemCount++] = LIRC_SPACE(NEXA_SYNC_PERIOD);

	return itemCount;
}
