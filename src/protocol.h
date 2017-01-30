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

#ifndef RFBB_PROTOCOL_H_
#define RFBB_PROTOCOL_H_

#include <stdbool.h>

int createNexaBitstream(const char *pHouseStr, const char *pChannelStr,
			const char *pOn_offStr, bool waveman, int32_t *txBitstream, int *repeatCount);

int createSartanoBitstream(const char *pChannelStr, const char *pOn_offStr, int32_t *txBitstream, int *repeatCount);
int createImpulsBitstream(const char *pChannelStr, const char *pOn_offStr, int32_t *txBitstream, int *repeatCount);
int createIkeaBitstream(const char *pSystemStr, const char *pChannelStr,
			const char *pLevelStr, const char *pDimStyle, int32_t *txBitstream, int *repeatCount);
int txBitstream2culStr(int32_t *pTxBitstream, int txItemCount, int repeatCount, char *txStrCul);

#endif /* RFBB_PROTOCOL_H_ */
