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

int createIkeaBitstream(const char *pSystemStr, const char *pChannelStr,
			const char *pLevelStr, const char *pDimStyle, int32_t *txBitstream, int *repeatCount)
{
#if 0
	*pStrReturn = '\0';	/* Make sure tx Bitstream is empty */

	const char STARTCODE[] = "STTTTTT�";
	const char TT[] = "TT";
	const char A[] = "�";
	int systemCode = atoi(pSystemStr) - 1;	/* System 1..16 */
	int channelCode = atoi(pChannelStr);	/* Channel 1..10 */
	int Level = atoi(pLevelStr);	/* off,10,20,..,90,on */
	int DimStyle = atoi(pDimStyle);
	int intCode = 0;
	int checksum1 = 0;
	int checksum2 = 0;
	int intFade;
	int i;
	int rawChannelCode = 0;

	/* check converted parameters for validity */
	if ((channelCode <= 0) || (channelCode > 10) || (systemCode < 0) || (systemCode > 15) ||
	    (Level < 0) || (Level > 10) || (DimStyle < 0) || (DimStyle > 1))
		return 0;

	if (channelCode == 10)
		channelCode = 0;
	rawChannelCode = (1 << (9 - channelCode));

	strcat(pStrReturn, STARTCODE);	//Startcode, always like this;
	intCode = (systemCode << 10) | rawChannelCode;

	for (i = 13; i >= 0; --i) {
		if ((intCode >> i) & 1) {
			strcat(pStrReturn, TT);
			if (i % 2 == 0)
				checksum2++;
			else
				checksum1++;
		} else {
			strcat(pStrReturn, A);
		}
	}

	if (checksum1 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//1st checksum

	if (checksum2 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//2nd checksum

	if (DimStyle == 1)
		intFade = 11 << 4;	//Smooth
	else
		intFade = 1 << 4;	//Instant

	switch (Level) {
	case 0:
		intCode = (10 | intFade);	//Concat level and fade
		break;

	case 1:
		intCode = (1 | intFade);	//Concat level and fade
		break;

	case 2:
		intCode = (2 | intFade);	//Concat level and fade
		break;

	case 3:
		intCode = (3 | intFade);	//Concat level and fade
		break;

	case 4:
		intCode = (4 | intFade);	//Concat level and fade
		break;

	case 5:
		intCode = (5 | intFade);	//Concat level and fade
		break;

	case 6:
		intCode = (6 | intFade);	//Concat level and fade
		break;

	case 7:
		intCode = (7 | intFade);	//Concat level and fade
		break;

	case 8:
		intCode = (8 | intFade);	//Concat level and fade
		break;

	case 9:
		intCode = (9 | intFade);	//Concat level and fade
		break;

	case 10:
	default:
		intCode = (0 | intFade);	//Concat level and fade
		break;
	}

	checksum1 = 0;
	checksum2 = 0;

	for (i = 0; i < 6; ++i) {
		if ((intCode >> i) & 1) {
			strcat(pStrReturn, TT);

			if (i % 2 == 0)
				checksum1++;
			else
				checksum2++;
		} else {
			strcat(pStrReturn, A);
		}
	}

	if (checksum1 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//2nd checksum

	if (checksum2 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//2nd checksum
	strcat(pStrReturn, "+");

	return strlen(pStrReturn);
#endif
	return 0;
}

