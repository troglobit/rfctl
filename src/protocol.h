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

#define DEFAULT_DEVICE "/dev/rfbb"

#define RF_MAX_TX_BITS 4000	/* Max TX pulse/space elements in one message */
#define RF_MAX_RX_BITS 4000	/* Max read RX pulse/space elements at one go */

typedef enum { MODE_UNKNOWN, MODE_READ, MODE_WRITE } rfMode_t;
typedef enum { IFC_UNKNOWN, IFC_RFBB, IFC_CUL, IFC_TELLSTICK } rfInterface_t;
typedef enum { PROT_UNKNOWN, PROT_RAW, PROT_NEXA, PROT_PROOVE, PROT_NEXA_L,
	PROT_SARTANO, PROT_WAVEMAN, PROT_IKEA, PROT_ESIC, PROT_IMPULS
} rfProtocol_t;

#define LIRC_MODE2_SPACE     0x00000000
#define LIRC_MODE2_PULSE     0x01000000
#define LIRC_MODE2_TIMEOUT   0x03000000

#define LIRC_VALUE_MASK      0x00FFFFFF
#define LIRC_MODE2_MASK      0xFF000000

#define LIRC_SPACE(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_SPACE)
#define LIRC_PULSE(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_PULSE)
#define LIRC_TIMEOUT(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_TIMEOUT)

#define LIRC_VALUE(val) ((val)&LIRC_VALUE_MASK)
#define LIRC_MODE2(val) ((val)&LIRC_MODE2_MASK)

#define LIRC_IS_SPACE(val) (LIRC_MODE2(val) == LIRC_MODE2_SPACE)
#define LIRC_IS_PULSE(val) (LIRC_MODE2(val) == LIRC_MODE2_PULSE)
#define LIRC_IS_TIMEOUT(val) (LIRC_MODE2(val) == LIRC_MODE2_TIMEOUT)

#define NEXA_SHORT_PERIOD 340	/* microseconds */
#define NEXA_LONG_PERIOD  1020	/* microseconds */
#define NEXA_SYNC_PERIOD  (32 * NEXA_SHORT_PERIOD)	/* between frames */
#define NEXA_REPEAT 4

#define SARTANO_SHORT_PERIOD 320	/* microseconds */
#define SARTANO_LONG_PERIOD  960	/* microseconds */
#define SARTANO_SYNC_PERIOD  (32 * SARTANO_SHORT_PERIOD)	/* between frames */
#define SARTANO_REPEAT 4


int createNexaBitstream(const char *pHouseStr, const char *pChannelStr,
			const char *pOn_offStr, bool waveman, int32_t *txBitstream, int *repeatCount);

int createSartanoBitstream(const char *pChannelStr, const char *pOn_offStr, int32_t *txBitstream, int *repeatCount);
int createImpulsBitstream(const char *pChannelStr, const char *pOn_offStr, int32_t *txBitstream, int *repeatCount);
int createIkeaBitstream(const char *pSystemStr, const char *pChannelStr,
			const char *pLevelStr, const char *pDimStyle, int32_t *txBitstream, int *repeatCount);
int txBitstream2culStr(int32_t *pTxBitstream, int txItemCount, int repeatCount, char *txStrCul);

#endif /* RFBB_PROTOCOL_H_ */
