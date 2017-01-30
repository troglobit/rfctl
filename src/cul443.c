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

/* Convert generic bitstream format to CUL433 format */
int txBitstream2culStr(int32_t *pTxBitstream, int txItemCount, int repeatCount, char *txStrCul)
{
	int i;
	int pulses = 0;
	char *pCulStr = txStrCul;
	char tmpStr[20];

	*pCulStr = '\0';

	strcat(pCulStr, "\r\nX01\r\n");	/* start radio */
	strcat(pCulStr, "E\r\n");	/* empty tx buffer */

	for (i = 0; i < txItemCount; i++) {
		sprintf(tmpStr, "%04X", LIRC_VALUE(pTxBitstream[i]));

		if (LIRC_IS_PULSE(pTxBitstream[i]) == true) {
			strcat(pCulStr, "A");
			strcat(pCulStr, tmpStr);
			pulses++;
		} else {	/* low */

			strcat(pCulStr, tmpStr);
			strcat(pCulStr, "\r\n");
		}
	}

	if (pulses > 1) {
		/* number of repetitions */
		sprintf(tmpStr, "S%02d\r\n", repeatCount);
		strcat(pCulStr, tmpStr);
		return strlen(pCulStr);
	}

	txStrCul[0] = '\0';
	return 0;
}
